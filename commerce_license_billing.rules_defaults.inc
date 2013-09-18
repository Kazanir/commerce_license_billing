<?php

/**
 * @file
 * Default rule configurations for Commerce License Billing.
 */

/**
 * Implements hook_default_rules_configuration().
 */
function commerce_license_billing_default_rules_configuration() {
  $items = array();
  $items['commerce_license_billing_set_postpaid_product_price'] = entity_import('rules_config', '{ "commerce_license_billing_set_postpaid_product_price" : {
      "LABEL" : "Set the price of postpaid products to 0",
      "PLUGIN" : "reaction rule",
      "REQUIRES" : [ "rules", "commerce_license_billing", "commerce_product_reference" ],
      "ON" : [ "commerce_product_calculate_sell_price" ],
      "IF" : [
        { "NOT entity_is_of_bundle" : {
            "entity" : [ "commerce-line-item" ],
            "type" : "commerce_line_item",
            "bundle" : { "value" : { "recurring" : "recurring" } }
          }
        },
        { "entity_has_field" : { "entity" : [ "commerce-line-item" ], "field" : "commerce_product" } },
        { "commerce_license_billing_product_is_postpaid" : { "commerce_product" : [ "commerce-line-item:commerce-product" ] } }
      ],
      "DO" : [
        { "commerce_line_item_unit_price_amount" : {
            "commerce_line_item" : [ "commerce_line_item" ],
            "amount" : "0",
            "component_name" : "base_price",
            "round_mode" : "0"
          }
        }
      ]
    }
  }');

  return $items;
}
