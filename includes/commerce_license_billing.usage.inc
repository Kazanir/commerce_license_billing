<?php

/**
 * @file
 * Additional reporting API functions for licenses with plan and usage history.
 */

/**
 * Gets all billing cycles which overlap the given range of timestamps.
 *
 * @param int $start A starting UNIX timestamp.
 *
 * @param int $end An ending UNIX timestamp.
 *
 * @return CommerceLicenseBillingCycleInterface[] A list of billing cycles
 *   keyed by cycle ID.
 */
function commerce_license_billing_get_cycles($start, $end) {

}

/**
 * For a given license and billing cycle, create a set of denormalized plan
 *   and usage history records which reflect the state of the license and its
 *   usage at any point during the cycle.
 *
 * @param CommerceLicenseInterface $license
 *
 * @param CommerceLicenseBillingCycleInterface $cycle
 *
 * @return array[] A list of denormalized plan and usage records for this
 *   license and billing cycle.
 */
function commerce_license_billing_denormalize_history($license, $cycle) {
  $usage_history = commerce_license_billing_usage_history_list($license, $cycle);
  $plan_history = commerce_license_billing_plan_history_list($license, $cycle);

  foreach ($plan_history as &$ph) {
    $ph['revision'] = NULL;
    if ($ph['end'] != 0) {
      $ending_stamps[] = $ph['end'];
    }
    $starting_stamps[] = $ph['start'];
  }
  unset($ph);

  foreach ($usage_history as $group_name => $usage_records) {
    foreach ($usage_records as $uh) {
      if ($uh['end'] != 0) {
        $ending_stamps[] = $uh['end'];
      }
      $starting_stamps[] = $uh['start'];
    }
  }

  $ending_stamps[] = $cycle->end;

  // Sort the ending timestamps so we hit them in order.
  sort($ending_stamps);
  sort($starting_stamps);
  $starting_stamps = array_unique($starting_stamps);
  $ending_stamps = array_unique($ending_stamps);

  $current_start = $cycle->start;

  foreach ($ending_stamps as $current_end) {
    unset($record);

    $record['license_id'] = $license->license_id;

    $count = 0;
    foreach ($plan_history as $ph) {
      // Look for a plan history entry which overlaps our chosen period.
      // There should only be one.
      if ($ph['start'] < $current_end && $ph['end'] > $current_start) {
        if ($count) {
          // There cannot be more than one of these. Throw.
          throw new LogicException("Found multiple plan history records for license ID $license->license_id between timestamps $current_start and $current_end.");
        }
        $count++;
        $record['revision_id'] = $ph['revision_id'];
        $record['product_id'] = $ph['product_id'];
      }
    }

    foreach ($usage_history as $group_name => $usage_records) {
      $count = 0;
      foreach ($usage_records as $uh) {
        // Look for a usage history entry which overlaps our chosen period.
        // There should only be one.
        if ($uh['start'] < $current_end && $uh['end'] > $current_start) {
          if ($count) {
            // There cannot be more than one of these. Throw.
            throw new LogicException("Found multiple usage history records for license ID $license->license_id and usage group $group_name between timestamps $current_start and $current_end.");
          }
          $count++;
          $record[$group_name] = $uh['quantity'];
        }
      }
    }
    $record['start_iso'] = date('c', $current_start);
    $record['end_iso'] = date('c', $current_end);
    // $record['start'] = $current_start;
    // $record['end'] = $current_end;

    $records[] = $record;

    // Figure out when the next start is.
    if (in_array($current_end + 1, $starting_stamps)) {
      $current_start = $current_end + 1;
    }
    else {
      // We're missing the next starting stamp, which should mean an inactivity
      // gap in the plan history.
      $filtered_starts = array_filter($starting_stamps, function ($stamp) use ($current_end) { return $stamp > $current_end; });
      $current_start = reset($filtered_starts);
    }
  }

  return $records;
}

