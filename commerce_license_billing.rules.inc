<?php
/**
 * @file
 * Rules integration for the Commerce License Billing module.
 */

/**
 * Implements hook_rules_condition_info().
 */
function commerce_license_billing_rules_condition_info() {
  $conditions['commerce_license_billing_product_elligible'] = array(
    'label' => t('Product is elligible for recurring'),
    'parameter' => array(
      'commerce_product' => array(
        'type' => 'commerce_product',
        'label' => t('Product'),
      ),
    ),
    'group' => t('Commerce License Billing'),
    'callbacks' => array(
      'execute' => 'commerce_license_billing_product_elligible_condition',
    ),
  );
  $conditions['commerce_license_billing_order_elligible'] = array(
    'label' => t('Order is elligible for recurring'),
    'parameter' => array(
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Order'),
      ),
    ),
    'group' => t('Commerce License Billing'),
    'callbacks' => array(
      'execute' => 'commerce_license_billing_order_elligible_condition',
    ),
  );

  return $conditions;
}

/**
 * Implements hook_rules_action_info().
 */
function commerce_license_billing_rules_action_info() {
  $actions['commerce_license_billing_create_recurring_orders'] = array(
    'label' => t('Create recurring orders based on the initial order'),
    'parameter' => array(
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Order'),
      ),
    ),
    'group' => t('Commerce License Billing'),
    'callbacks' => array(
      'execute' => 'commerce_license_billing_create_recurring_orders_action',
    ),
  );

  return $actions;
}

/**
 * Rules condition callback: check if a product is elligible for recurring.
 */
function commerce_license_billing_product_elligible_condition($product) {
  return !empty($product->cl_billing_cycle_type);
}

/**
 * Rules condition callback: check if an order is elligible for recurring.
 */
function commerce_license_billing_order_elligible_condition($order) {
  // Prevent recursion.
  if ($order->type == 'recurring') {
    return FALSE;
  }

  // Make sure the order has at least one license & billing cycle type selected.
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $line_item_wrapper) {
    if (!empty($line_item_wrapper->commerce_license)) {
      $product = $line_item_wrapper->commerce_product->value();
      if (!empty($product->cl_billing_cycle_type)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
 * Rules action callback: create recurring orders based on the initial order.
 */
function commerce_license_billing_create_recurring_orders_action($order) {
  commerce_license_billing_create_recurring_orders($order);
}
