<?php

/**
 * A test billing cycle engine API.
 *
 * Always returns the same async billing cycle, exactly 30 days long.
 */
class CommerceLicenseBillingCycleTypeTest extends CommerceLicenseBillingCycleTypeBase {

  /**
   * Returns the user's billing cycle with the provided start time.
   *
   * If an existing billing cycle matches the expected start and end, it will
   * be returned. Otherwise, a new one will be created.
   *
   * @param $uid
   *   The uid of the user.
   * @param $start
   *   The unix timestamp when the billing cycle needs to start.
   * @param $save
   *   Whether to save the created billing cycle entity.
   *   Passing FALSE allows an unsaved billing cycle entity to be returned
   *   for estimation purposes.
   *
   * @return
   *   A cl_billing_cycle entity.
   */
  public function getBillingCycle($uid, $start = REQUEST_TIME, $save = TRUE) {
    // Make the billing cycle exactly 30 days long, so that it can be divided
    // predictably for prorating.
    // The 1 is substracted to make sure that the billing cycle ends 1s before
    // the next one starts
    $end = ($start + 2592000) - 1;

    // Try to find an existing billing cycle matching our parameters.
    $query = new EntityFieldQuery;
    $query
      ->entityCondition('entity_type', 'cl_billing_cycle')
      ->entityCondition('bundle', $this->name)
      ->propertyCondition('status', 1)
      ->propertyCondition('uid', $uid);
    if ($start != REQUEST_TIME) {
      // In case of a custom start, make sure to match the exact billing cycle.
      // Ensures that new orders get the previous billing cycle created at the
      // start of testing, while getNextBillingCycle returns the expected result.
      $query->propertyCondition('start', $start);
    }
    $result = $query->execute();
    if ($result) {
      $billing_cycle_id = key($result['cl_billing_cycle']);
      $billing_cycle = entity_load_single('cl_billing_cycle', $billing_cycle_id);
    }
    else {
      // No existing billing cycle found. Create a new one.
      $billing_cycle = entity_create('cl_billing_cycle', array('type' => $this->name));
      $billing_cycle->uid = $uid;
      $billing_cycle->start = $start;
      $billing_cycle->end = $end;
      $billing_cycle->status = 1;
      if ($save) {
        $billing_cycle->save();
      }
    }

    return $billing_cycle;
  }

  /**
   * Returns the user's next billing cycle.
   *
   * @param $billing_cycle
   *   The current billing cycle.
   * @param $save
   *   Whether to save the created billing cycle entity.
   *   Passing FALSE allows an unsaved billing cycle entity to be returned
   *   for estimation purposes.
   *
   * @return
   *   A cl_billing_cycle entity.
   */
  public function getNextBillingCycle($billing_cycle, $save = TRUE) {
    return $this->getBillingCycle($billing_cycle->uid, $billing_cycle->end + 1, $save);
  }

  /**
   * Returns a label for a billing cycle with the provided start and end.
   *
   * @param $start
   *   The unix timestmap when the billing cycle starts.
   * @param $end
   *   The unix timestamp when the billing cycle ends.
   *
   * @return
   *   The billing cycle label.
   */
  public function getBillingCycleLabel($start, $end) {
    return date('F jS Y', $start) . ' - ' . date('F jS Y', $end);
  }
}
