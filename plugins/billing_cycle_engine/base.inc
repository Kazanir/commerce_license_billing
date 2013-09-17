<?php

/**
 * Ensures basic required behavior for a billing cycle engine.
 *
 * EntityBundlePluginProvideFieldsInterface also mandates a fields() method.
 */
interface CommerceLicenseBillingCycleEngineInterface extends EntityBundlePluginProvideFieldsInterface {

  /**
   * Returns the user's billing cycle with the provided start time.
   *
   * If an existing billing cycle matches the expected start and end, it will
   * be returned. Otherwise, a new one will be created.
   *
   * @param $uid
   *   The uid of the user.
   * @param $start
   *   The unix timestamp when the billing cycle needs to start.
   * @param $save
   *   Whether to save the created billing cycle entity.
   *   Passing FALSE allows an unsaved billing cycle entity to be returned
   *   for estimation purposes.
   *
   * @return
   *   A cl_billing_cycle entity.
   */
  public function getBillingCycle($uid, $start = REQUEST_TIME, $save = TRUE);

  /**
   * Returns the user's next billing cycle.
   *
   * @param $billing_cycle
   *   The current billing cycle.
   * @param $save
   *   Whether to save the created billing cycle entity.
   *   Passing FALSE allows an unsaved billing cycle entity to be returned
   *   for estimation purposes.
   *
   * @return
   *   A cl_billing_cycle entity.
   */
  public function getNextBillingCycle($billing_cycle, $save = TRUE);

  /**
   * Returns a label for the billing cycle with the provided start and end.
   *
   * @param $start
   *   The unix timestmap when the billing cycle starts.
   * @param $end
   *   The unix timestamp when the billing cycle ends.
   *
   * @return
   *   The billing cycle label.
   */
  public function getBillingCycleLabel($start, $end);

}

/**
 * Billing cycle type base class.
 */
abstract class CommerceLicenseBillingCycleTypeBase extends Entity implements CommerceLicenseBillingCycleEngineInterface {

  /**
   * The billing cycle type id.
   *
   * @var integer
   */
  public $billing_cycle_type_id;

  /**
   * The billing cycle engine (bundle).
   *
   * @var string
   */
  public $engine;

  /**
   * The machine-readable name of this billing cycle type.
   *
   * @var string
   */
  public $name;

  /**
   * The human-readable name of the billing cycle type.
   *
   * @var string
   */
  public $title;

  /**
   * Whether the generated billing cycles will be prepaid or postpaid.
   *
   * @var boolean
   */
  public $prepaid;

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
  public function __construct(array $values, $entityType = NULL) {
    parent::__construct($values, 'cl_billing_cycle_type');
    $this->wrapper = entity_metadata_wrapper($this->entityType, $this);
  }

  /**
   * Implements EntityBundlePluginProvideFieldsInterface::fields().
   */
  static function fields() {
    return array();
  }
}
