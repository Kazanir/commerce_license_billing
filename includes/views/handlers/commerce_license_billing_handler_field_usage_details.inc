<?php

/**
 * Field handler to return usage details for a license.
 */
class commerce_license_billing_handler_field_usage_details extends views_handler_field {
  function construct() {
    parent::construct();

    $this->additional_fields['license_id'] = 'license_id';
  }

  function query() {
    $this->ensure_my_table();
    $this->add_additional_fields();
  }

  function render($values) {
    $license_id = $this->get_value($values, 'license_id');
    $license = entity_load_single('commerce_license', $license_id);
    if ($license instanceof CommerceLicenseBillingUsageInterface) {
      return $license->usageDetails();
    }
  }
}
