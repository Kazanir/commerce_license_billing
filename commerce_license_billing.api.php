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
