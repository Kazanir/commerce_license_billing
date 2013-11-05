<?php

/**
 * Ensures basic required behavior for a usage group.
 */
interface CommerceLicenseBillingUsageGroupInterface {

  /**
   * Adds a usage record for the given license revision.
   *
   * @param $revisionId
   *   The revision_id of the license. Used to price the usage according to the
   *   active plan.
   * @param $quantity
   *   The usage quantity.
   * @param $start
   *   Unix timestamp indicating when the usage started.
   * @param $end
   *   Unix timestamp indicating when the usage ended.
   */
  public function addUsage($revisionId, $quantity, $start, $end);

  /**
   * Returns the current usage quantity.
   */
  public function currentUsage();

  /**
   * Returns the usage history for the current billing cycle.
   *
   * @param $billing_cycle
   *   The billing cycle entity.
   */
  public function usageHistory(CommerceLicenseBillingCycle $billingCycle);

  /**
   * Returns the chargeable usage records for the current billing cycle.
   *
   * @param $billing_cycle
   *   The billing cycle entity.
   */
  public function chargeableUsage(CommerceLicenseBillingCycle $billingCycle);

  /**
   * Returns whether all usage has been reported for the current billing cycle.
   *
   * @param $billing_cycle
   *   The active billing cycle.
   *
   * @return
   *   TRUE if all usage has been reported for each plan in the current
   *   billing cycle, FALSE otherwise.
   */
  public function isComplete(CommerceLicenseBillingCycle $billingCycle);

  /**
   * Reacts on license revision changes.
   *
   * A new revision is created when the license status or plan change,
   * both of might require matching changes in usage.
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
  public function onRevisionChange();

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
   * Implements CommerceLicenseBillingUsageGroupInterface::addUsage().
   */
  public function addUsage($revisionId, $quantity, $start, $end) {
    // Make sure that usage is being registered for an active revision.
    $revision = entity_revision_load('commerce_license', $revisionId);
    if ($revision->status != COMMERCE_LICENSE_ACTIVE) {
      throw new Exception('Tried to register usage for a non-active revision #' . $revisionId);
    }

    // Add the usage record.
    db_insert('cl_billing_usage')
      ->fields(array(
        'license_id' => $this->license->license_id,
        'revision_id' => $revisionId,
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
   */
  public function usageHistory(CommerceLicenseBillingCycle $billingCycle) {
    // The procedural function has its own static cache, allowing the history
    // records to be loaded just once per license, not once per group.
    $usage_history = commerce_license_billing_usage_history_list($this->license, $billingCycle);
    return !empty($usage_history[$this->groupName]) ? $usage_history[$this->groupName] : array();
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::isComplete().
   */
  public function isComplete(CommerceLicenseBillingCycle $billingCycle) {
    $plan_history = commerce_license_billing_plan_history_list($this->license, $billingCycle);
    $is_complete = TRUE;
    foreach ($plan_history as $history_record) {
      $usage_history = $this->usageHistory($billingCycle);
      $remaining_gap = ($history_record['end'] - $history_record['start']);
      foreach ($usage_history as $usage_record) {
        $remaining_gap -= ($usage_record['end'] - $usage_record['start'] + 1);
      }

      if ($remaining_gap > 0) {
        $is_complete = FALSE;
        break;
      }
    }

    return $is_complete;
  }

  /**
   * Implements CommerceLicenseBillingUsageGroupInterface::onRevisionChange().
   */
  public function onRevisionChange() {}

  /**
   * Returns free usage quantities for each plan in the current billing cycle.
   *
   * @param $billing_cycle
   *   The billing cycle entity.
   *
   * @return
   *   An array of free quantity amounts, keyed by the plan's revision_id.
   */
  protected function freeQuantities(CommerceLicenseBillingCycle $billingCycle) {
    $free_quantities = array();
    // Start with the free usage given by the current license revision.
    $free_quantities[$this->license->revision_id] = $this->groupInfo['free_quantity'];
    // Now get the free usage given by all other referenced revisions.
    $plan_history = commerce_license_billing_plan_history_list($this->license, $billingCycle);
    foreach ($plan_history as $history_record) {
      if ($history_record['revision_id'] != $this->license->revision_id) {
        $revision = entity_revision_load('commerce_license', $history_record['revision_id']);
        $revision_usage_groups = $revision->usageGroups();
        $revision_group_info = $revision_usage_groups[$this->groupName];
        $free_quantities[$history_record['revision_id']] = $revision_group_info['free_quantity'];
      }
    }

    return $free_quantities;
  }
}
