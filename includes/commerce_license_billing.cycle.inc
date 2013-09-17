<?php

/**
 * The class for the billing cycle entity.
 */
class CommerceLicenseBillingCycle extends Entity {

  /**
   * The billing cycle id.
   *
   * @var integer
   */
  public $billing_cycle_id;

  /**
   * The billing cycle type (bundle).
   *
   * @var string
   */
  public $type;

  /**
   * The uid of the billing cycle owner.
   *
   * @var integer
   */
  public $uid;

  /**
   * Whether the billing cycle is open or closed.
   *
   * @var boolean
   */
  public $status;

  /**
   * The date (unix timestamp) when the billing cycle starts.
   *
   * @var integer
   */
  public $start;

  /**
   * The date (unix timestamp) when the billing cycle ends.
   *
   * @var integer
   */
  public $end;

  /**
   * Metadata wrapper.
   *
   * @var EntityDrupalWrapper
   */
  public $wrapper;

  /**
   * Constructor.
   *
   * @see Entity::__construct()
   */
  public function __construct(array $values) {
    parent::__construct($values, 'cl_billing_cycle');
    $this->wrapper = entity_metadata_wrapper($this->entityType, $this);
  }

  /**
   * Overrides Entity::label().
   */
  public function label() {
    $type = entity_load_single('cl_billing_cycle_type', $this->type);
    return $type->getBillingCycleLabel($this->start, $this->end);
  }
}
