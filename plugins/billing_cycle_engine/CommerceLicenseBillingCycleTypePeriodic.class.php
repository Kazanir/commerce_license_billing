<?php

/**
 * A periodic billing cycle engine API.
 */
class CommerceLicenseBillingCycleTypePeriodic extends CommerceLicenseBillingCycleTypeBase {

  /**
   * Implements EntityBundlePluginProvideFieldsInterface::fields().
   */
  static function fields() {
    $fields['pce_period']['field'] = array(
      'type' => 'list_text',
      'cardinality' => '1',
      'translatable' => '0',
      'settings' => array(
        'allowed_values' => array(
          'hour' => 'Hour',
          'day' => 'Day',
          'week' => 'Week',
          'month' => 'Month',
          'quarter' => 'Quarter',
          'half-year' => 'Half-year',
          'year' => 'Year',
        ),
      ),
    );
    $fields['pce_period']['instance'] = array(
      'label' => 'Period',
      'description' => 'Determines the length of a generated billing cycle.',
      'required' => TRUE,
      'widget' => array(
        'module' => 'options',
        'settings' => array(),
        'type' => 'options_select',
      ),
    );
    $fields['pce_async']['field'] = array(
      'type' => 'list_boolean',
      'cardinality' => '1',
      'translatable' => '0',
      'settings' => array(
        'allowed_values' => array(
          0 => 'Synchronous',
          1 => 'Asynchronous'
        ),
      ),
    );
    $fields['pce_async']['instance'] = array(
      'label' => 'Asynchronous',
      'required' => FALSE,
      'widget' => array(
        'module' => 'options',
        'type' => 'options_onoff',
        'settings' => array(
          'display_label' => FALSE,
         ),
        'weight' => 399,
      ),
    );
    return $fields;
  }

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
    $period = $this->wrapper->pce_period->value();
    if (!$this->wrapper->pce_async->value()) {
      // This is a synchronous billing cycle, normalize the start timestamp.
      switch ($period) {
        case 'hour':
          $day = date('d', $start);
          $month = date('m', $start);
          $year = date('Y', $start);
          $hour = date('H', $start);
          $start = mktime($hour, 0, 0, $month, $day, $year);
          break;
        case 'day':
          $start = strtotime('today');
          break;
        case 'week':
          $day = date('d', $start);
          $month = date('m', $start);
          $year = date('Y', $start);
          $start = strtotime('this week', mktime(0, 0, 0, $month, $day, $year));
          break;
        case 'month':
          $start = strtotime(date('F Y', $start));
          break;
        case 'quarter':
          $year = date('Y', $start);
          $dates = array(
            mktime(0, 0, 0, 1, 1, $year), // january 1st
            mktime(0, 0, 0, 4, 1, $year), // april 1st,
            mktime(0, 0, 0, 7, 1, $year), // july 1st
            mktime(0, 0, 0, 10, 1, $year), // october 1st,
            mktime(0, 0, 0, 1, 1, $year + 1),
          );

          foreach ($dates as $index => $date) {
            if ($start >= $date && $start < $dates[$index + 1]) {
              $start = $date;
              break;
            }
          }
          break;
        case 'half-year':
          $year = date('Y', $start);
          $january1st = mktime(0, 0, 0, 1, 1, $year);
          $july1st = mktime(0, 0, 0, 7, 1, $year);
          $start = ($start < $july1st) ? $january1st : $july1st;
          break;
        case 'year':
          $start = mktime(0, 0, 0, 1, 1, date('Y', $start) + 1);
          break;
      }
    }
    // Calculate the end timestamp.
    $period_mapping = array(
      'hour' => '+1 hour',
      'day' => '+1 day',
      'week' => '+1 week',
      'month' => '+1 month',
      'quarter' => '+3 months',
      'half-year' => '+6 months',
      'year' => '+1 year',
    );
    // The 1 is subtracted to make sure that the billing cycle ends 1s before
    // the next one starts (January 31st 23:59:59, for instance, with the
    // next one starting on February 1st 00:00:00).
    $end = strtotime($period_mapping[$period], $start) - 1;

    // Try to find an existing billing cycle matching our parameters.
    $query = new EntityFieldQuery;
    $query
      ->entityCondition('entity_type', 'cl_billing_cycle')
      ->entityCondition('bundle', $this->name)
      ->propertyCondition('uid', $uid)
      ->propertyCondition('start', $start)
      ->propertyCondition('end', $end);
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
   *   The unix timestamap when the billing cycle starts.
   * @param $end
   *   The unix timestamp when the billing cycle ends.
   *
   * @return
   *   The billing cycle label.
   */
  public function getBillingCycleLabel($start, $end) {
    $async = $this->wrapper->pce_async->value();
    $period = $this->wrapper->pce_period->value();
    // Example: January 15th 2013
    if ($period == 'day') {
      return date('F jS Y', $end);
    }

    if ($async) {
      // Example: January 1st 2013 - January 31st 2013.
      return date('F jS Y', $start) . ' - ' . date('F jS Y', $end);
    }
    else {
      if ($period == 'week') {
        // Example: January 1st 2013 - January 7th 2013.
        return date('F jS Y', $start) . ' - ' . date('F jS Y', $end);
      }
      elseif ($period == 'month') {
        // Example: January 2013.
        return date('F Y');
      }
    }
  }
}
