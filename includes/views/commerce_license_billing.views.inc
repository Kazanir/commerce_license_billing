<?php

/**
 * Provides views integration for commerce_license_billing.
 */

/**
 * Implements hook_views_data_alter().
 */
function commerce_license_billing_views_data_alter(&$data) {
  $data['commerce_license']['usage_details'] = array(
    'field' => array(
      'title' => t('Usage details'),
      'help' => t('Display the current usage for the license.'),
      'handler' => 'commerce_license_billing_handler_field_usage_details',
    ),
  );
}
