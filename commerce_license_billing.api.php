<?php

/**
 * @file
 * Hooks provided by the Commerce License Billing module.
 */

/**
 * Returns the initial usage for a license's usage group.
 *
 * Initial usage is registered when the license is first activated.
 * The first hook to provide initial usage wins. If no hooks are found,
 * the 'initial_quantity' key on group info.
 *
 * Right now initial usage is only registered for gauge usage groups.
 *
 * @param $license
 *   The license entity.
 * @param $group_name
 *   The name of the usage group, as defined in $license->usageGroups().
 *
 * @return
 *   The numeric quantity.
 */
function hook_commerce_license_billing_initial_usage($license, $group_name) {
  if ($group_name == 'environments') {
    // In this example the initial usage is stored on the line item because
    // the customer selected the desired number of environments during checkout.
    $query = new EntityFieldQuery;
    $query
      ->entityCondition('entity_type', 'commerce_line_item')
      ->entityCondition('bundle', 'recurring', '<>')
      ->fieldCondition('commerce_license', 'target_id', $license->license_id);
    $result = $query->execute();
    if ($result) {
      $line_item_id = key($result['commerce_line_item']);
      $line_item = commerce_line_item_load($line_item_id);
      $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
      return $line_item_wrapper->field_user_environments->value();
    }
  }
}

/**
 * Alter the estimated charges for a license.
 *
 * The estimated charges show how an imaginary recurring order would look
 * like if it charged for the provided plan and usage.
 * Each charge can be seen as an imaginary line item.
 *
 * This hook allows modules that do order-level pricing logic (adding a sales
 * tax charge, for example), thus simulating a real order more closely.
 *
 * @param array $estimation
 *   An array of estimated charges, with the following keys:
 *   - total: The estimated total for all charges.
 *   - charges: An array of charges with the following keys:
 *     - product: The commerce_product entity for which the charge was generated.
 *     - quantity: The quantity.
 *     - unit_price: The unit price of the charge,
 *     - total: The total price of a charge.
 *   Each unit price or total is a commerce_price array struct.
 * @param array $context
 *   An array with the following keys:
 *   - license: Stub license referencing the requested plan and the owner uid.
 *   - billing_cycle: Stub billing cycle.
 *   - usage: Expected usage (in the usage_group => quantity format), or NULL.
 *   - order_id: The order_id used in the calculation process. This can be
 *     the ID of an active recurring order, or the ID of a cart, or 0.
 *
 *   Note that while drupal_alter hooks normally take references, this function
 *   should not be used to modify these entities as some of them
 *   (license, billing cycle) are stubs.
 */
function hook_commerce_license_billing_estimation_alter(&$estimation, $context) {

}

/**
 * Alter a newly created recurring order.
 *
 * Allows modules to transfer custom fields from the previous order to the
 * new one.
 *
 * Note that licenses attached to the order can be fetched using
 * commerce_license_billing_get_recurring_order_licenses($order).
 *
 * @param $recurring_order
 *   The newly created recurring order.
 * @param $previous_order
 *   The previous order based on which the current one is generated.
 *   This can be the initial non-recurring order or a recurring order for
 *   the previous billing cycle.
 */
function hook_commerce_license_billing_new_recurring_order_alter($recurring_order, $previous_order) {
  // We have a custom field_foo on our orders that needs to be retained on each
  // subsequent recurring order.
  if (isset($previous_order->field_foo)) {
    $recurring_order->field_foo = $previous_order->field_foo;
  }
}
