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
  $actions['commerce_license_billing_prorate_line_item'] = array(
    'label' => t('Prorate line item'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
      ),
    ),
    'group' => t('Commerce License Billing'),
    'callbacks' => array(
      'execute' => 'commerce_license_billing_prorate_line_item_action',
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

/**
 * Rules action callback: Prorate line item.
 */
function commerce_license_billing_prorate_line_item_action($line_item) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $product_wrapper = $line_item_wrapper->commerce_product;
  $product = $product_wrapper->value();
  if (empty($product->cl_billing_cycle_type)) {
    // The product that is not elligible for recurring.
    return;
  }

  // Construct a possible billing cycle for this product.
  $time = commerce_license_get_time();
  $billing_cycle_type = $product_wrapper->cl_billing_cycle_type->value();
  $billing_cycle = $billing_cycle_type->getBillingCycle(0, $time, FALSE);
  // Construct a charge that starts now and ends with the billing cycle.
  $charges = array();
  $charges[0] = array(
    'commerce_product' => $product,
    'quantity' => 1,
    'commerce_unit_price' => $product_wrapper->commerce_price->value(),
    'cl_billing_start' => $time,
    'cl_billing_end' => $billing_cycle->end,
    'prorate' => TRUE,
  );
  commerce_license_billing_prorate_charges($charges, $billing_cycle);

  // Set the prorated amount on the line item.
  $unit_price = commerce_price_wrapper_value($line_item_wrapper, 'commerce_unit_price', TRUE);
  $updated_amount = $charges[0]['commerce_unit_price']['amount'];
  $current_amount = $unit_price['amount'];
  $difference = array(
    'amount' => $updated_amount - $current_amount,
    'currency_code' => $unit_price['currency_code'],
    'data' => array(),
  );

  // Set the amount of the unit price and add the difference as a component.
  $line_item_wrapper->commerce_unit_price->amount = $updated_amount;
  $line_item_wrapper->commerce_unit_price->data = commerce_price_component_add(
    $line_item_wrapper->commerce_unit_price->value(),
    'base_price',
    $difference,
    TRUE
  );
}
