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
  $conditions['commerce_license_billing_product_prorating_eligible'] = array(
    'label' => t('Product is eligible for prorating'),
    'parameter' => array(
      'commerce_product' => array(
        'type' => 'commerce_product',
        'label' => t('Product'),
      ),
    ),
    'group' => t('Commerce License Billing'),
    'callbacks' => array(
      'execute' => 'commerce_license_billing_product_prorating_eligible_condition',
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
  $actions['commerce_license_billing_prorate_product_line_item'] = array(
    'label' => t('Prorate a product line item'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
      ),
    ),
    'group' => t('Commerce License Billing'),
    'callbacks' => array(
      'execute' => 'commerce_license_billing_prorate_product_line_item_action',
    ),
  );
  $actions['commerce_license_billing_prorate_recurring_line_item'] = array(
    'label' => t('Prorate a recurring line item'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
      ),
    ),
    'group' => t('Commerce License Billing'),
    'callbacks' => array(
      'execute' => 'commerce_license_billing_prorate_recurring_line_item_action',
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
 * Rules condition calback: check if a product is eligible for prorating.
 *
 * Postpaid plan products and usage products need to be prorated.
 */
function commerce_license_billing_product_prorating_eligible_condition($product) {
  $eligible = FALSE;
  // These products are missing the standard billing cycle fields or they are empty,
  // meaning they are being used solely as usage products.
  if (empty($product->cl_billing_cycle_type) || empty($product->cl_billing_type)) {
    $eligible = TRUE;
  }
  else {
    $wrapper = entity_metadata_wrapper('commerce_product', $product);
    if ($wrapper->cl_billing_type->value() == 'postpaid') {
      $eligible = TRUE;
    }
  }

  return $eligible;
}

/**
 * Rules action callback: create recurring orders based on the initial order.
 */
function commerce_license_billing_create_recurring_orders_action($order) {
  commerce_license_billing_create_recurring_orders($order);
}

/**
 * Rules action callback: Prorate a product line item.
 */
function commerce_license_billing_prorate_product_line_item_action($line_item) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $product_wrapper = $line_item_wrapper->commerce_product;
  $product = $product_wrapper->value();
  if (empty($product->cl_billing_cycle_type)) {
    // The product is not elligible for recurring.
    return;
  }

  // Construct a possible billing cycle for this product.
  $time = commerce_license_get_time();
  $billing_cycle_type = $product_wrapper->cl_billing_cycle_type->value();
  $billing_cycle = $billing_cycle_type->getBillingCycle(0, $time, FALSE);
  // Prorate the price.
  $duration = $billing_cycle->end - $time;
  commerce_license_billing_prorate_line_item($line_item_wrapper, $duration, $billing_cycle);
}

/**
 * Rules action callback: Prorate a recurring line item.
 */
function commerce_license_billing_prorate_recurring_line_item_action($line_item) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $billing_start = $line_item_wrapper->cl_billing_start->value();
  $billing_end = $line_item_wrapper->cl_billing_end->value();
  $duration = $billing_end - $billing_start;
  $billing_cycle = $line_item_wrapper->order->cl_billing_cycle->value();
  commerce_license_billing_prorate_line_item($line_item_wrapper, $duration, $billing_cycle);
}

/**
 * Prorates the unit price of a line item.
 *
 * @param $line_item_wrapper
 *   The line item wrapper.
 * @param $duration
 *   The duration of the line item.
 * @param $billing_cycle
 *   The billing cycle entity.
 */
function commerce_license_billing_prorate_line_item($line_item_wrapper, $duration, $billing_cycle) {
  $unit_price = commerce_price_wrapper_value($line_item_wrapper, 'commerce_unit_price', TRUE);
  if ($unit_price['amount'] > 0) {
    $billing_cycle_duration = $billing_cycle->end - $billing_cycle->start;
    $prorated_amount = $unit_price['amount'] * ($duration / $billing_cycle_duration);
    $prorated_amount = round($prorated_amount);

    // Set the prorated amount as the unit price and add the difference as a
    // component.
    $difference = array(
      'amount' => $prorated_amount - $unit_price['amount'],
      'currency_code' => $unit_price['currency_code'],
      'data' => array(),
    );
    $line_item_wrapper->commerce_unit_price->amount = $prorated_amount;
    $line_item_wrapper->commerce_unit_price->data = commerce_price_component_add(
      $line_item_wrapper->commerce_unit_price->value(),
      'base_price',
      $difference,
      TRUE
    );
  }
}
