<?php

/**
 * @file
 * Provides Entity metadata integration.
 */

class CommerceLicenseBillingCycleMetadataController extends EntityDefaultMetadataController {

  /**
   * Overrides EntityDefaultMetadataController::entityPropertyInfo().
   */
  public function entityPropertyInfo() {
    $info = array();
    $properties = &$info['cl_billing_cycle']['properties'];

    $properties['billing_cycle_id'] = array(
      'type' => 'integer',
      'label' => t('Billing cycle ID'),
      'description' => t('The primary identifier for a billing cycle..'),
      'validation callback' => 'entity_metadata_validate_integer_positive',
      'schema field' => 'billing_cycle_id',
    );
    $properties['type'] = array(
      'type' => 'token',
      'label' => t('Type'),
      'description' => t('The billing cycle type.'),
      'options list' => 'CommerceLicenseMetadataController::bundleOptionsList',
      'setter callback' => 'entity_property_verbatim_set',
      'required' => TRUE,
      'schema field' => 'type',
    );
    $properties['owner'] = array(
      'type' => 'user',
      'label' => t('Owner'),
      'description' => t('The owner of the billing cycle.'),
      'setter callback' => 'entity_property_verbatim_set',
      'required' => TRUE,
      'schema field' => 'uid',
    );
    $properties['status'] = array(
      'type' => 'boolean',
      'label' => t('Status'),
      'description' => t('The billing cycle status.'),
      'setter callback' => 'entity_property_verbatim_set',
      'required' => TRUE,
      'schema field' => 'status',
    );
    $properties['start'] = array(
      'type' => 'date',
      'label' => t('Start'),
      'description' => t('The date when the billing cycle starts.'),
      'schema field' => 'start',
    );
    $properties['end'] = array(
      'type' => 'date',
      'label' => t('End'),
      'description' => t('The date when the billing cycle ends.'),
      'schema field' => 'end',
    );

    return $info;
  }
}
