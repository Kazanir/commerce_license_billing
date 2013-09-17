<?php
/**
 * @file
 * Admin UI for Billing cycle types.
 */

/**
 * UI controller.
 */
class CommerceLicenseBillingCycleTypeUIController extends EntityDefaultUIController {

  public function __construct($entity_type, $entity_info) {
    parent::__construct($entity_type, $entity_info);
    // Shorten the entity type label.
    $this->entityInfo['label'] = t('Billing cycle type');
  }

  /**
   * Overrides EntityDefaultUIController::hook_menu().
   */
  public function hook_menu() {
    $items = parent::hook_menu();
    // Make the listing show up as a tab.
    $items[$this->path]['type'] = MENU_LOCAL_TASK;
    $items[$this->path]['weight'] = 100;
    // Replace the title callback of pages.
    $title_callback = 'commerce_license_billing_cycle_type_get_page_title';
    $items[$this->path . '/add']['title callback'] = $title_callback;
    $items[$this->path . '/import']['title callback'] = $title_callback;
    // Replace entity_ui_get_form() with a custom function.
    $page_callback = 'commerce_license_billing_cycle_type_get_form';
    $items[$this->path . '/add']['page callback'] = $page_callback;
    $items[$this->path . '/add']['page arguments'] = array(NULL, 'add');
    $manage_path = $this->path . '/manage/%entity_object';
    $items[$manage_path]['page callback'] = $page_callback;
    $items[$manage_path]['page arguments'] = array($this->id_count + 1);
    $items[$manage_path . '/clone']['page callback'] = $page_callback;
    $items[$manage_path . '/clone']['page arguments'] = array($this->id_count + 1, 'clone');

    return $items;
  }
}

/**
 * Builds the billing cycle type entity form.
 *
 * Based on entity_ui_get_form(), with two differences:
 * - For the 'add' action the 'periodic' billing engine is preselected.
 * - commerce_license_billing_cycle_type_form() is used instead of
 *   cl_billing_cycle_type_form().
 */
function commerce_license_billing_cycle_type_get_form($billing_cycle_type, $op = 'edit') {
  // Preselect the 'periodic' billing engine on the add form.
  if (!isset($billing_cycle_type) && $op == 'add') {
    $billing_cycle_type = entity_create('cl_billing_cycle_type', array('engine' => 'periodic'));
  }
  // Handle cloning. We cannot do that in the wrapper callback as it is too late
  // for changing arguments.
  if ($op == 'clone') {
    $billing_cycle_type = entity_ui_clone_entity('cl_billing_cycle_type', $billing_cycle_type);
  }

  // Do not use drupal_get_form(), but invoke drupal_build_form() ourself so
  // we can prepulate the form state.
  $form_state = array();
  $form_state['wrapper_callback'] = 'entity_ui_main_form_defaults';
  $form_state['entity_type'] = 'cl_billing_cycle_type';
  form_load_include($form_state, 'inc', 'entity', 'includes/entity.ui');

  // We don't pass the entity type as first parameter, as the implementing
  // module knows the type anyway. However, in order to allow for efficient
  // hook_forms() implementiations we append the entity type as last argument,
  // which the module implementing the form constructor may safely ignore.
  // @see entity_forms()
  $form_state['build_info']['args'] = array($billing_cycle_type, $op, 'cl_billing_cycle_type');
  return drupal_build_form('commerce_license_billing_cycle_type_form', $form_state);
}

/**
 * Gets the page title for the passed operation.
 */
function commerce_license_billing_cycle_type_get_page_title($op) {
  switch ($op) {
    case 'add':
      return t('Add billing cycle type');
    case 'import':
      return t('Import billing cycle type');
    case 'edit':
      return t('Edit billing cycle type');
    case 'clone':
      return t('Clone billing cycle type');
    case 'revert':
      return t('Revert billing cycle type');
    case 'delete':
      return t('Delete billing cycle type');
    case 'export':
      return t('Export billing cycle type');
  }
}

/**
 * Billing cycle type form.
 */
function commerce_license_billing_cycle_type_form($form, &$form_state, $billing_cycle_type, $op = 'edit') {
  global $user;
  $is_new = !empty($billing_cycle_type->is_new) ? $billing_cycle_type->is_new : FALSE;

  // Set the title.
  drupal_set_title(commerce_license_billing_cycle_type_get_page_title($op), PASS_THROUGH);

  // Add the base properties.
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => !empty($billing_cycle_type->title) ? $billing_cycle_type->title : '',
    '#maxlength' => 32,
    '#required' => TRUE,
    '#weight' => -10,
  );
  $form['name'] = array(
    '#type' => 'machine_name',
    '#title' => t('Machine name'),
    '#default_value' => !empty($billing_cycle_type->name) ? $billing_cycle_type->name : '',
    '#maxlength' => 255,
    '#machine_name' => array(
      'exists' => 'commerce_license_billing_cycle_type_name_exists',
      'source' => array('title'),
    ),
    '#disabled' => $is_new && $user->uid != 1,
    '#weight' => -9,
  );

  // Add the field related form elements.
  $form['#entity_type'] = 'cl_billing_cycle_type';
  $form_state['cl_billing_cycle_type'] = $billing_cycle_type;

  field_attach_form('cl_billing_cycle_type', $billing_cycle_type, $form, $form_state);

  $form['actions'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('form-actions')),
    '#weight' => 400,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => $is_new ? t('Create') : t('Save'),
    '#submit' => (!empty($form['#submit']) ? $form['#submit'] : array()) + array('commerce_license_billing_cycle_type_form_submit'),
  );

  return $form;
}

/**
 * Submit callback for commerce_license_billing_cycle_type_form().
 */
function commerce_license_billing_cycle_type_form_submit($form, &$form_state) {
  $entity_type = $form['#entity_type'];
  $entity = $form_state[$entity_type];

  // Notify field widgets.
  entity_form_submit_build_entity($entity_type, $entity, $form, $form_state);
  entity_save($entity_type, $entity);
  drupal_set_message(t('Billing cycle type %name saved.', array('%name' => $entity->title)));
  drupal_goto('admin/commerce/config/license/billing-cycle-types');
}

/**
 * Checks whether a cycle type with the provided machine name already exists.
 */
function commerce_license_billing_cycle_type_name_exists($machine_name) {
  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'cl_billing_cycle_type')
    ->propertyCondition('name', $machine_name)
    ->count();
  $count = $query->execute();
  return ($count > 0);
}
