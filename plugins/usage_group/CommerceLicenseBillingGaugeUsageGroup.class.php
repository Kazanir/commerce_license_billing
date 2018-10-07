<?php

/**
 * Gauge usage group.
 *
 * Prorates and charges each discrete value for each plan in a billing cycle.
 */
class CommerceLicenseBillingGaugeUsageGroup extends CommerceLicenseBillingUsageGroupBase {

  /**
   * Overrides CommerceLicenseBillingUsageBase::addUsage().
   *
   * Gauge usage behaves differently than counter usage. Because gauge usage
   * measures periods of time and has to cover the entire span of the plan
   * during a given billing cycle, this function needs to ensure that no
   * existing usage records exist for the requested time period.
   *
   * A new gauge usage record will always "take over" the requested time frame,
   * removing other usage records which occupy the same timeframe and adjusting
   * those which overlap to "make space" for the new usage record.
   *
   * The default behavior ends up being intuitive if only start times are used,
   * but the results are consistent even if specific time periods are inserted
   * out of sequence or need to be updated after their original insertion.
   */
  public function addUsage($quantity, $start = NULL, $end = NULL) {
    if ($start === NULL) {
      // Default $start to current time.
      $start = commerce_license_get_time();
    }

    $txn = db_transaction();

    // Look for a record which overlaps the new usage group from the start
    // side. This gets updated to align with the new record's start time.
    db_update('cl_billing_usage')
      ->fields(array(
        'end' => ($start - 1),
      ))
      ->condition('license_id', $this->license->license_id)
      ->condition('usage_group', $this->groupName)
      ->condition(db_or()
        ->condition('end', 0)
        ->condition('end', $start, ">=")
      )
      ->condition('start', $start, "<")
      ->execute();

    if ($end == 0) {
      // Since there is no end for the new usage, we need to delete all records
      // which start on or after the requested start time.
      db_delete('cl_billing_usage')
        ->condition('license_id', $this->license->license_id)
        ->condition('usage_group', $this->groupName)
        ->condition('start', $start, ">=")
        ->execute();
    }
    else {
      // Look for any usage records inside of the new record and delete them.
      db_delete('cl_billing_usage')
        ->condition('license_id', $this->license->license_id)
        ->condition('usage_group', $this->groupName)
        ->condition('start', $start, ">=")
        ->condition('end', $end, "<=")
        ->execute();

      // Look for a usage record which overlaps the new record from the end
      // side. Update this to have a start immediately after the new record's
      // end time.
      db_update('cl_billing_usage')
        ->fields(array(
          'start' => ($end + 1),
        ))
        ->condition('license_id', $this->license->license_id)
        ->condition('usage_group', $this->groupName)
        ->condition(db_or()
          ->condition('start', $start, ">=")
          ->condition('end', $end, ">")
        )
      ->execute();
    }

    // Once this is all finished we insert the new record normally.
    parent::addUsage($quantity, $start, $end);

    unset($txn);
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::currentUsage().
   */
  public function currentUsage($billingCycle = NULL) {
    if (is_null($billingCycle)) {
      // Default to the current billing cycle.
      $billingCycle = commerce_license_billing_get_license_billing_cycle($this->license);
      if (!$billingCycle) {
        // A billing cycle could not be found, possibly because the license
        // hasn't been activated yet.
        return 0;
      }
    }

    // Get the quantity of the usage record with the highest usage_id
    // for the current billing cycle.
    $current_usage = 0;
    $previous_usage_id = 0;
    $usage = $this->usageHistory($billingCycle);
    foreach ($usage as $usage_record) {
      if ($usage_record['usage_id'] > $previous_usage_id) {
        $current_usage = $usage_record['quantity'];
        $previous_usage_id = $usage_record['usage_id'];
      }
    }

    return $current_usage;
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::chargeableUsage().
   */
  public function chargeableUsage(CommerceLicenseBillingCycle $billingCycle) {
    if (!empty($this->groupInfo['not_charged'])) {
      // This usage group shouldn't be charged.
      return array();
    }

    $usage = $this->usageHistory($billingCycle);
    $free_quantities = $this->freeQuantities($billingCycle);
    // Remove any usage that is free according to the active plan.
    foreach ($usage as $index => $usage_record) {
      $revision_id = $usage_record['revision_id'];
      if ($usage_record['quantity'] <= $free_quantities[$revision_id]['quantity']) {
        unset($usage[$index]);
      }
      else {
        $usage[$index]['quantity'] -= $free_quantities[$revision_id]['quantity'];
      }
    }

    return $usage;
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::onRevisionChange().
   */
  public function onLicenseChange() {
    $previous_status = $this->license->original->status;
    $new_status = $this->license->status;
    $current_time = commerce_license_get_time();
    // The license was activated for the first time. Register initial usage.
    if ($previous_status < COMMERCE_LICENSE_ACTIVE && $new_status == COMMERCE_LICENSE_ACTIVE) {
      $initial_usage = $this->initialUsage();
      if (!is_null($initial_usage)) {
        $this->addUsage($initial_usage, $current_time);
      }
    }
    // A new revision was created, and the previous revision was active.
    // Close previous open usage, reopen for the new revision if still active.
    elseif ($previous_status == COMMERCE_LICENSE_ACTIVE) {
      // Get the open usage for the previous revision. We can't use
      // $this->currentUsage() because it looks at the current revision instead.
      $data = array(
        ':group_name' => $this->groupName,
        ':revision_id' => $this->license->original->revision_id,
      );
      $query = db_query('SELECT quantity FROM {cl_billing_usage}
                            WHERE usage_group = :group_name
                              AND revision_id = :revision_id
                                AND end = 0
                                  ORDER BY usage_id DESC
                                    LIMIT 1', $data);
      $previous_usage = $query->fetchField();

      // Close the open usage for the previous revision (plan).
      db_update('cl_billing_usage')
        ->fields(array(
          'end' => $current_time - 1,
        ))
        ->condition('revision_id', $this->license->original->revision_id)
        ->condition('usage_group', $this->groupName)
        ->condition('end', '0')
        ->execute();

      // Reset the usage history static cache.
      drupal_static_reset('commerce_license_billing_usage_history_list');

      // If the license is still active, reopen the usage.
      if ($new_status == COMMERCE_LICENSE_ACTIVE && is_numeric($previous_usage)) {
        $this->addUsage($previous_usage, $current_time);
      }
    }
    // A new revision has been created, unsuspending the license. Reopen
    // previous usage.
    elseif ($previous_status == COMMERCE_LICENSE_SUSPENDED && $new_status == COMMERCE_LICENSE_ACTIVE) {
      // Get the last closed usage quantity for this group.
      $data = array(
        ':group_name' => $this->groupName,
        ':license_id' => $this->license->license_id,
      );
      $query = db_query('SELECT quantity FROM {cl_billing_usage}
                            WHERE usage_group = :group_name
                              AND license_id = :license_id
                                ORDER BY usage_id DESC
                                  LIMIT 1', $data);
      // If there is no previous usage quantity (for some reason) then we need
      // to default to 0 -- we can't send FALSE to addUsage because it will be
      // parametrized as '' and break the db_insert.
      $previous_quantity = ($query->fetchField() ?: 0);
      $this->addUsage($previous_quantity, $current_time);
    }
  }

  /**
   * Returns the initial usage.
   *
   * @return
   *   The initial usage to register, or NULL if none found.
   */
  protected function initialUsage() {
    // Try to get the initial usage from a hook.
    $usage_hook = 'commerce_license_billing_initial_usage';
    $initial_usage = NULL;
    foreach (module_implements($usage_hook) as $module) {
      $initial_usage = module_invoke($module, $usage_hook, $this->license, $this->groupName);
      if (!is_null($initial_usage)) {
        // Usage found, stop the search here.
        break;
      }
    }
    // If no module provided the initial usage, try looking at the group info.
    if (is_null($initial_usage) && !empty($this->groupInfo['initial_quantity'])) {
      $initial_usage = $this->groupInfo['initial_quantity'];
    }

    return $initial_usage;
  }
}
