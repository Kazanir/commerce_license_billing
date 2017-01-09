<?php

interface CommerceLicenseBillingChargeInterface extends ArrayAccess {
  public function match($line_item_wrapper);

  public function populate($line_item_wrapper);
}

class CommerceLicenseBillingCharge implements CommerceLicenseBillingChargeInterface {

  const SCHEMA = [
    'usage_group',
    'quantity',
    'commerce_product',
    'commerce_unit_price',
    'license_revision_id',
    'cl_billing_start',
    'cl_billing_end',
    'match',
  ];

  protected $container = array();

  public function __construct(array $def) {
    foreach ($def as $k => $v) {
      if (!in_array($k, static::SCHEMA)) {
        throw new DomainException("This charge type does not support the $k data element.");
      }
      $this->container[$k] = $v;
    }

    // Fill in the blanks.
    foreach (static::SCHEMA as $k) {
      if (!isset($this->container[$k])) {
        $this->container[$k] = NULL;
      }
    }
  }

  public function match($line_item_wrapper) {
    if (empty($this->container['match'])) {
      throw new LogicException("Unable to match line items as no matching elements are defined for this charge.");
    }
    $match = TRUE;
    foreach ($this->container['match'] as $key) {
      if (!in_array($key, static::SCHEMA)) {
        throw new DomainException("Trying to match data value not in the schema for this charge type.");
      }

      if ($line_item_wrapper->{$key}->value() != $this->container[$key]) {
        $match = FALSE;
        break;
      }
    }

    return $match;
  }

  public function populate($line_item_wrapper) {
    $line_item = $line_item_wrapper->value();
    $line_item_wrapper->line_item_label = $this->container['commerce_product']->sku;
    $line_item_wrapper->commerce_product = $this->container['commerce_product'];
    $line_item_wrapper->quantity = $this->container['quantity'];
    $line_item_wrapper->commerce_unit_price = $this->container['commerce_unit_price'];
    // We strip out price and product details, and retain the rest of the charge
    // on the price component.
    $base_component = $line_item_wrapper->commerce_unit_price->value();
    unset($this->container['commerce_product'], $this->container['commerce_unit_price']);
    // Charges are now stored on the line item's data array directly rather
    // than on a price component which might be accidentally changed by pricing
    // rules. This is left for BC reasons but users relying on the charge data
    // should move to the new property_info implementation.
    $base_component['data'] = array(
      'charge' => $this,
    );
    $line_item->data['charge'] = $this;

    $line_item_wrapper->commerce_unit_price->data = commerce_price_component_add(
      $line_item_wrapper->commerce_unit_price->value(),
      'base_price',
      $base_component,
      TRUE
    );
    $line_item->cl_billing_start[LANGUAGE_NONE][0]['value'] = date("Y-m-d H:i:s", $this->container['cl_billing_start']);
    $line_item->cl_billing_end[LANGUAGE_NONE][0]['value'] = date("Y-m-d H:i:s", $this->container['cl_billing_end']);
    // Store the revision ID of the license used to generate the charge, if any.
    // With the change to store the generating charge directly on the line item
    // this is no longer necessary, but is left for BC reasons.
    if (!empty($this->container['license_revision_id'])) {
      $line_item->data['license_revision_id'] = $this->container['license_revision_id'];
    }
  }

  public function offsetSet($offset, $value) {
    if (is_null($offset)) {
      throw new DomainException("This charge type does not support unmapped data elements.");
    }
    elseif (!in_array($offset, static::SCHEMA)) {
      throw new DomainException("This charge type does not support the $offset data element.");
    }
    else {
      $this->container[$offset] = $value;
    }
  }

  public function offsetExists($offset) {
    return isset($this->container[$offset]);
  }

  public function offsetUnset($offset) {
    unset($this->container[$offset]);
  }

  public function offsetGet($offset) {
    return isset($this->container[$offset]) ? $this->container[$offset] : null;
  }

}
