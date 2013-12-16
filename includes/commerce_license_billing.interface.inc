<?php

/**
 * Allows a license to track its usage for billing purposes.
 */
interface CommerceLicenseBillingUsageInterface {

  /**
   * Returns the usage groups for the current license type.
   *
   * A license type can have one or multiple usage groups, used to track
   * the corresponding types of usage.
   *
   * Each group must have the following keys:
   * - title: The name of the usage group, usually used by usageDetails().
   * - type: The type of the usage group (counter or gauge). Determines
   *   how the usage will be calculated, and subsequently charged for.
   *   The counter tracks usage over time, and is always charged for
   *   in total. For example if the following bandwidth usage was reported:
   *   "Jan 1st - Jan 15th; 1024" and "Jan 15th - Jan 31st; 128", there
   *   will be one line item, charging for 1052mb of usage.
   *   The gauge tracks discrete usage values over time, and each
   *   value is charged for separately. For example, if the following env
   *   usage is reported "Jan 1st - Jan 15th; 2" and "Jan 15th - Jan 31st; 4",
   *   there will be two prorated line items, charging for 2 and 4 environments.
   * - product: The sku of the product representing the usage group. Linked
   *   from the line item that charges for the usage.
   * Optional keys:
   * - free_quantity: The quantity provided for free with the license. Only
   *   usage exceeding this quantity will be charged for.
   *   For type 'counter' this means that the free quantity is subtracted
   *   from the total quantity. For type 'gauge' this means that the gauge
   *   values that equal free_quantity are ignored.
   * Optional keys for the 'gauge' type.
   * - not_charged: If TRUE, no charges for this usage group will be added to
   *   the order. Useful for use cases that require usage to be tracked, but
   *   where the charging happens through other methods (i.e. sublicenses).
   * - immediate: Whether usage changes are guaranteed to be reported right
   *   away. If TRUE, the usage is open-ended and will be carried over into next
   *   billing cycles.
   * - initial_quantity: The initial quantity to register when opening the
   *   billing cycle. For example, if the user starts with 1 development
   *   environment, "initial_quantity" should be set to 1.
   *
   * <code>
   * return array(
   *   'bandwidth' => array(
   *     'title' => t('Bandwidth'),
   *     'type' => 'counter',
   *     'product' => 'BILLING-TEST-BANDWIDTH',
   *     'free_quantity' => 0,
   *   ),
   *   'environments' => array(
   *     'title' => t('Development environments'),
   *     'type' => 'gauge',
   *     'product' => 'BILLING-TEST-ENV',
   *     'immediate' => TRUE,
   *     // Take the number of free environments from the plan.
   *     'free_quantity' => $this->wrapper->product->free_environments->value(),
   *   ),
   * );
   * </code>
   */
  public function usageGroups();

  /**
   * Returns the usage details for the current license type.
   *
   * Usually implemented by iterating over usageGroups(), calculating usage
   * for each group, and outputting it to the user in a human-readable way.
   *
   * @return
   *   An html string with the usage details.
   */
  public function usageDetails();

}
