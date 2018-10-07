<?php

/**
 * Ensures basic required behavior for a usage group.
 */
interface CommerceLicenseBillingUsageGroupInterface {

  /**
   * Enforces change scheduling for the given license property.
   *
   * This means that any change to the given property ("status", for example)
   * must be scheduled for the end of the billing cycle, so that it doesn't
   * interfere with billing.
   *
   * @param $property
   *   The license property that will be changed, usually "product" or "status".
   *
   * @return
   *   TRUE if changes to the given property must be scheduled, FALSE otherwise.
   */
  public function enforceChangeScheduling($property);

  /**
   * Adds a usage record for the given license revision.
   *
   * @param mixed $quantity
   *   The usage quantity.
   * @param int|NULL $start
   *   Unix timestamp indicating when the usage started.
   * @param int|NULL $end
   *   Unix timestamp indicating when the usage ended, or NULL if it has not
   *   ended.
   */
  public function addUsage($quantity, $start = NULL, $end = NULL);

  /**
   * Returns the usage quantity for the provided / current billing cycle.
   *
   * @param $billing_cycle
   *   The billing cycle for which usage should be returned.
   *   If not provided, the current billing cycle is assumed.
   */
  public function currentUsage($billingCycle = NULL);

  /**
   * Returns the usage history for the provided billing cycle.
   *
   * @param $billing_cycle
   *   The billing cycle.
   */
  public function usageHistory(CommerceLicenseBillingCycle $billingCycle);

  /**
   * Returns the chargeable usage records for the provided billing cycle.
   *
   * @param $billing_cycle
   *   The billing cycle.
   */
  public function chargeableUsage(CommerceLicenseBillingCycle $billingCycle);

  /**
   * Returns whether all usage has been reported for the provided billing cycle.
   *
   * @param $billing_cycle
   *   The billing cycle.
   *
   * @return
   *   TRUE if all usage has been reported for each plan in the provided
   *   billing cycle, FALSE otherwise.
   */
  public function isComplete(CommerceLicenseBillingCycle $billingCycle);

  /**
   * Reacts on license changes.
   *
   * This is triggered when the license status or plan change, both of might
   * require matching changes in usage.
   *
   * This event has access to the previous license in $this->license->original.
   * Key data:
   *   - previous revision: $this->license->original->revision_id
   *   - current revision: $this->license->revision_id
   *   - previous plan: $this->license->original->product_id
   *   - current plan: $this->license->product_id
   *   - previous status: $this->license->original->status
   *   - new status: $this->license->status
   */
  public function onLicenseChange();

}

/**
 * Usage group base class.
 */
abstract class CommerceLicenseBillingUsageGroupBase implements CommerceLicenseBillingUsageGroupInterface {

  /**
   * The for which the usage is being recorded.
   *
   * @var CommerceLicenseBase
   */
  protected $license;

  /**
   * The name of the usage group.
   *
   * @var string
   */
  protected $groupName;

  /**
   * The usage group information as defined in $license->usageGroups().
   *
   * @var array
   */
  protected $groupInfo;

  public function __construct($license, $groupName, $groupInfo) {
    $this->license = $license;
    $this->groupName = $groupName;
    $this->groupInfo = $groupInfo;
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::enforceChangeScheduling().
   */
  public function enforceChangeScheduling($property) {
    return FALSE;
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::addUsage().
   */
  public function addUsage($quantity, $start = NULL, $end = NULL) {
    if ($start === NULL) {
      // Default $start to current time.
      $start = commerce_license_get_time();
    }

    // Add the usage record.
    db_insert('cl_billing_usage')
      ->fields(array(
        'license_id' => $this->license->license_id,
        'usage_group' => $this->groupName,
        'quantity' => $quantity,
        'start' => $start,
        'end' => $end,
      ))
      ->execute();

    // Reset the usage history static cache.
    drupal_static_reset('commerce_license_billing_usage_history_list');
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::usageHistory().
   *
   * @todo make this take ($start, $end) instead of $billingCycle, so it can look at usage for a plan record's start/end dates
   */
  public function usageHistory(CommerceLicenseBillingCycle $billingCycle) {
    // The procedural function has its own static cache, allowing the history
    // records to be loaded just once per license, not once per group.
    $usage_history = commerce_license_billing_usage_history_list($this->license, $billingCycle->start, $billingCycle->end);
    return !empty($usage_history[$this->groupName]) ? $usage_history[$this->groupName] : array();
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::isComplete().
   */
  public function isComplete(CommerceLicenseBillingCycle $billingCycle) {
    // If the usage records are complete, then the sum of their durations
    // will be the same as the sum of plan durations.
    // Every plan or usage record ends a second before the next one, so
    // the equation needs to add 1s to each record to get the correct duration.
    $plan_history = commerce_license_billing_plan_history_list($this->license, $billingCycle->start, $billingCycle->end);
    $plan_duration = 0;
    foreach ($plan_history as $history_record) {
      $plan_duration += ($history_record['end'] - $history_record['start'] + 1);
    }
    $usage_history = $this->usageHistory($billingCycle);
    $usage_duration = 0;
    foreach ($usage_history as $usage_record) {
      $usage_duration += ($usage_record['end'] - $usage_record['start'] + 1);
    }

    return $plan_duration == $usage_duration;
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::onRevisionChange().
   */
  public function onLicenseChange() {}

  /**
   * Returns free usage quantities for each plan in the current billing cycle.
   *
   * @param CommerceLicenseBillingCycle $billing_cycle
   *   The billing cycle entity.
   *
   * @todo replace with a method that returns a single free quantity for a given ($start, $end)
   *
   * @return array
   *   An array of arrays keyed by revision id, with the keys:
   *   - quantity: The free quantity for this plan.
   *   - start: The start timestamp of the plan.
   *   - end: The end timestamp of the plan.
   */
  protected function freeQuantities(CommerceLicenseBillingCycle $billingCycle) {
    $free_quantities = array();
    $plan_history = commerce_license_billing_plan_history_list($this->license, $billingCycle->start, $billingCycle->end);
    $usage_groups = $this->license->usageGroups();
    // @todo rewrite this to select the revision for the start/end times specified, and return the right product
    foreach ($plan_history as $history_record) {
      $free_quantities[] = array(
        'quantity' => $usage_groups[$this->groupName]['free_quantity'],
        'start' => $history_record['start'],
        'end' => $history_record['end'],
      );
    }

    return $free_quantities;
  }
}
