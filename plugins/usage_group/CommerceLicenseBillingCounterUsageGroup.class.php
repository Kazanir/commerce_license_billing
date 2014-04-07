<?php

/**
 * Counter usage group.
 *
 * Aggregates and charges the total for each plan in a billing cycle.
 */
class CommerceLicenseBillingCounterUsageGroup extends CommerceLicenseBillingUsageGroupBase {

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

    // Sum up all usage for the current billing cycle and revisions up to
    // (and including) the current one.
    $current_usage = 0;
    $usage = $this->usageHistory($billingCycle);
    foreach ($usage as $usage_record) {
      if ($usage_record['revision_id'] <= $this->license->revision_id) {
        $current_usage += $usage_record['quantity'];
      }
    }

    return $current_usage;
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::chargeableUsage().
   *
   * Since counter usage is charged for in total, the counter usage records are
   * collapsed into one record per usage group.
   */
  public function chargeableUsage(CommerceLicenseBillingCycle $billingCycle) {
    $chargeable_usage = array();
    $usage = $this->usageHistory($billingCycle);
    $free_quantities = $this->freeQuantities($billingCycle);
    $billing_cycle_duration = $billingCycle->end - $billingCycle->start;

    // There could be multiple records per revision, so group them first.
    $counter_totals = array();
    foreach ($usage as $index => $usage_record) {
      $revision_id = $usage_record['revision_id'];
      // Initialize the counter.
      if (!isset($counter_totals[$revision_id])) {
        $counter_totals[$revision_id] = 0;
      }
      $counter_totals[$revision_id] += $usage_record['quantity'];
    }
    // Now compare the totals with the free quantities for each revision, and
    // create the final total that has only the non-free quantities.
    $total = 0;
    foreach ($counter_totals as $revision_id => $quantity) {
      // Prorate the free quantity. So if the free quantity is "10", but
      // the plan only spanned half of the billing cycle, the actual
      // free quantity will be "5".
      $free_quantity = $free_quantities[$revision_id];
      $free_quantity_duration = ($free_quantity['end'] - $free_quantity['start']);
      $free_quantity_amount =  $free_quantity['quantity'] * ($free_quantity_duration / $billing_cycle_duration);
      $free_quantity_amount = round($free_quantity_amount);

      if ($quantity > $free_quantity_amount) {
        $total += ($quantity - $free_quantity_amount);
      }
    }

    if ($total > 0) {
      $chargeable_usage[] = array(
        'usage_group' => $this->groupName,
        'quantity' => $total,
        'start' => $billingCycle->start,
        'end' => $billingCycle->end,
      );
    }

    return $chargeable_usage;
  }
}
