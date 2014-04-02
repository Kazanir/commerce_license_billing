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

  /**
   * Returns the matching order for this billing cycle.
   *
   * @return
   *   An order entity, or FALSE if none found.
   */
  public function getOrder() {
    $query = new EntityFieldQuery();
    $query
      ->entityCondition('entity_type', 'commerce_order')
      ->entityCondition('bundle', 'recurring')
      ->fieldCondition('cl_billing_cycle', 'target_id', $this->billing_cycle_id);
    $results = $query->execute();
    if (!empty($results['commerce_order'])) {
      $order_id = key($results['commerce_order']);
      return commerce_order_load($order_id);
    }
    else {
      return FALSE;
    }
  }
}
