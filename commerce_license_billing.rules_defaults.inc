<?php

/**
 * @file
 * Default rule configurations for Commerce License Billing.
 */

/**
 * Implements hook_default_rules_configuration_alter().
 */
function commerce_license_billing_default_rules_configuration_alter(&$configs) {
  // The order account rules run at commerce_checkout_complete, which is usually
  // after commerce_payment_order_paid_in_full. This means that if the checkout
  // was anonymous, a recurring order can't be opened because the order owner
  // is not yet known. Until this is fixed in Commerce, this module needs to
  // ensure that these rules run at commerce_payment_order_paid_in_full as well.
  $configuration_names = array(
    'commerce_checkout_order_convert',
    'commerce_checkout_new_account',
  );

  foreach ($configuration_names as $configuration_name) {
    if (isset($configs[$configuration_name])) {
      $configs[$configuration_name]->event('commerce_payment_order_paid_in_full');
    }
  }
}

/**
 * Implements hook_default_rules_configuration().
 */
function commerce_license_billing_default_rules_configuration() {
  $items = array();
  $items['commerce_license_billing_set_postpaid_product_price'] = entity_import('rules_config', '{ "commerce_license_billing_set_postpaid_product_price" : {
      "LABEL" : "Set the price of postpaid products to 0",
      "PLUGIN" : "reaction rule",
      "WEIGHT" : "-5",
      "OWNER" : "rules",
      "TAGS" : [ "Commerce License Billing" ],
      "REQUIRES" : [
        "rules",
        "commerce_license_billing",
        "commerce_line_item",
        "commerce_product_reference"
      ],
      "ON" : { "commerce_product_calculate_sell_price" : [] },
      "IF" : [
        { "NOT entity_is_of_bundle" : {
            "entity" : [ "commerce-line-item" ],
            "type" : "commerce_line_item",
            "bundle" : { "value" : { "recurring" : "recurring" } }
          }
        },
        { "entity_has_field" : { "entity" : [ "commerce-line-item" ], "field" : "commerce_product" } },
        { "entity_has_field" : {
            "entity" : [ "commerce-line-item:commerce-product" ],
            "field" : "cl_billing_type"
          }
        },
        { "commerce_license_billing_product_elligible" : { "commerce_product" : [ "commerce-line-item:commerce-product" ] } },
        { "data_is" : {
            "data" : [ "commerce-line-item:commerce-product:cl-billing-type" ],
            "value" : "postpaid"
          }
        }
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
  $items['commerce_license_billing_prorate_prepaid_product_price'] = entity_import('rules_config', '{ "commerce_license_billing_prorate_prepaid_product_price" : {
      "LABEL" : "Prorate the price of prepaid products",
      "PLUGIN" : "reaction rule",
      "WEIGHT" : "-5",
      "OWNER" : "rules",
      "TAGS" : [ "Commerce License Billing" ],
      "REQUIRES" : [ "rules", "commerce_license_billing", "commerce_product_reference" ],
      "ON" : { "commerce_product_calculate_sell_price" : [] },
      "IF" : [
        { "NOT entity_is_of_bundle" : {
            "entity" : [ "commerce-line-item" ],
            "type" : "commerce_line_item",
            "bundle" : { "value" : { "recurring" : "recurring" } }
          }
        },
        { "entity_has_field" : { "entity" : [ "commerce-line-item" ], "field" : "commerce_product" } },
        { "commerce_license_billing_product_elligible" : { "commerce_product" : [ "commerce-line-item:commerce-product" ] } },
        { "entity_has_field" : {
            "entity" : [ "commerce-line-item:commerce-product" ],
            "field" : "cl_billing_type"
          }
        },
        { "data_is" : {
            "data" : [ "commerce-line-item:commerce-product:cl-billing-type" ],
            "value" : "prepaid"
          }
        },
        { "entity_has_field" : {
            "entity" : [ "commerce-line-item:commerce-product" ],
            "field" : "cl_billing_cycle_type"
          }
        },
        { "entity_has_field" : {
            "entity" : [ "commerce-line-item:commerce-product:cl-billing-cycle-type" ],
            "field" : "pce_async"
          }
        },
        { "data_is" : {
            "data" : [ "commerce-line-item:commerce-product:cl-billing-cycle-type:pce-async" ],
            "value" : 0
          }
        }
      ],
      "DO" : [
        { "commerce_license_billing_prorate_line_item" : { "commerce_line_item" : [ "commerce_line_item" ] } }
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
    $items['rules_commerce_license_billing_update_order_charged'] = entity_import('rules_config', '{ "rules_commerce_license_billing_update_order_charged" : {
        "LABEL" : "Update status of successfully charged recurring order",
        "PLUGIN" : "reaction rule",
        "WEIGHT" : "11",
        "TAGS" : [ "Commerce License Billing" ],
        "REQUIRES" : [ "commerce_order", "commerce_cardonfile" ],
        "ON" : [ "commerce_cardonfile_charge_success" ],
        "DO" : [
          { "commerce_order_update_status" : {
              "commerce_order" : [ "order" ],
              "order_status" : "completed"
            }
          }
        ]
      }
    }');
  }

  return $items;
}
