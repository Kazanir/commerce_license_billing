<?php

/**
 * Billing test license type.
 */
class CommerceLicenseBillingTest extends CommerceLicenseBase implements CommerceLicenseBillingUsageInterface  {

  /**
   * Implements CommerceLicenseBillingUsageInterface::usageGroups().
   */
  public function usageGroups() {
    return array(
      // Track GBs of bandwidth used.
      'bandwidth' => array(
        'title' => t('Bandwidth'),
        'type' => 'counter',
        'product' => 'BILLING-TEST-BANDWIDTH',
        'free_quantity' => 1024,
      ),
      'environments' => array(
        'title' => t('Development environments'),
        'type' => 'gauge',
        'product' => 'BILLING-TEST-ENV',
        'immediate' => TRUE,
        'free_quantity' => 3,
      ),
    );
  }

  /**
   * Implements CommerceLicenseBillingUsageInterface::usageDetails().
   */
  public function usageDetails() {
    $bandwidth_usage = commerce_license_billing_current_usage($this, 'bandwidth');
    $env_usage = commerce_license_billing_current_usage($this, 'environments');

    $details = t('Bandwidth: @bandwidth GB', array('@bandwidth' => $bandwidth_usage));
    $details .= '<br />';
    $details .= t('Environments: @environments', array('@environments' => $env_usage));
    return $details;
  }

  /**
   * Implements CommerceLicenseInterface::checkoutCompletionMessage().
   */
  public function checkoutCompletionMessage() {
    $text = 'Thank you for purchasing the billing test.';
    return $text;
  }
}
