<?php
/**
 * @file
 * Rules integration for the Commerce License Billing module.
 */

/**
 * Implements hook_rules_condition_info().
 */
function commerce_license_billing_rules_condition_info() {
  $conditions['commerce_license_billing_product_is_postpaid'] = array(
    'label' => t('Product is postpaid'),
    'parameter' => array(
      'commerce_product' => array(
        'type' => 'commerce_product',
        'label' => t('Product'),
      ),
    ),
    'group' => t('Commerce License Billing'),
    'callbacks' => array(
      'execute' => 'commerce_license_billing_product_postpaid_condition',
    ),
  );
  return $conditions;
}

/**
 * Rules condition callback: check if the product is postpaid.
 */
function commerce_license_billing_product_postpaid_condition($product) {
  // Only return TRUE if the product has a billing cycle type selected
  // (meaning that it has enabled billing).
  if (!empty($product->cl_billing_cycle_type)) {
    $product_wrapper = entity_metadata_wrapper('commerce_product', $product);
    if ($product_wrapper->cl_billing_type->value() == 'postpaid') {
      return TRUE;
    }
  }

  return FALSE;
}
