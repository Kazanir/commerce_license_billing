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
  public function currentUsage() {
    $data = array(
      ':license_id' => $this->license->license_id,
      ':group' => $this->groupName,
    );
    $usage = db_query("SELECT SUM(quantity) FROM {cl_billing_usage}
                    WHERE license_id = :license_id AND usage_group = :group
                      GROUP BY license_id, usage_group", $data)->fetchColumn();

    return $usage;
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
      $free_quantity = $free_quantities[$revision_id];
      if ($quantity > $free_quantity) {
        $total += ($quantity - $free_quantity);
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
