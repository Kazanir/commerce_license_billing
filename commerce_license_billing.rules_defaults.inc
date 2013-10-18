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
      "TAGS" : [ "Commerce License Billing" ],
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
  $items['commerce_license_billing_create_recurring_orders'] = entity_import('rules_config', '{ "commerce_license_billing_create_recurring_orders" : {
      "LABEL" : "Create recurring orders based on the initial order",
      "PLUGIN" : "reaction rule",
      "WEIGHT" : "10",
      "TAGS" : [ "Commerce License Billing" ],
      "REQUIRES" : [ "commerce_license_billing", "commerce_payment" ],
      "ON" : [ "commerce_payment_order_paid_in_full" ],
      "IF" : [
        { "commerce_license_billing_order_elligible" : { "commerce_order" : [ "commerce_order" ] } }
      ],
      "DO" : [
        { "commerce_license_billing_create_recurring_orders" : { "commerce_order" : [ "commerce_order" ] } }
      ]
    }
  }');
  // For some reason, a crash occurs if the cardonfile module is missing when
  // adding a default rule with it as the dependency.
  if (module_exists('commerce_cardonfile')) {
    $items['rules_commerce_license_billing_charge_recurring_order'] = entity_import('rules_config', '{ "rules_commerce_license_billing_charge_recurring_order" : {
        "LABEL" : "Charge a recurring order",
        "PLUGIN" : "reaction rule",
        "TAGS" : [ "Commerce License Billing" ],
        "REQUIRES" : [ "rules", "commerce_payment", "commerce_cardonfile", "entity" ],
        "ON" : [ "commerce_order_update" ],
        "IF" : [
          { "NOT data_is" : {
              "data" : [ "commerce-order:status" ],
              "value" : [ "commerce-order-unchanged:status" ]
            }
          },
          { "data_is" : {
              "data" : [ "commerce-order:status" ],
              "value" : "recurring_payment_pending"
            }
          },
          { "commerce_payment_order_balance_comparison" : {
              "commerce_order" : [ "commerce_order" ],
              "operator" : "\u003E",
              "value" : "0"
            }
          }
        ],
        "DO" : [
          { "commerce_cardonfile_order_select_card" : {
              "USING" : { "order" : [ "commerce-order" ] },
              "PROVIDE" : { "select_card_response" : { "select_card_response" : "Select card response" } }
            }
          },
          { "commerce_cardonfile_order_charge_card" : {
              "USING" : {
                "order" : [ "commerce-order" ],
                "charge" : [ "" ],
                "select_card_response" : [ "select_card_response" ],
                "card_data" : [ "" ]
              },
              "PROVIDE" : { "charge_card_response" : { "charge_card_response" : "charge Card Response" } }
            }
          }
        ]
      }
    }');
  }

  return $items;
}
