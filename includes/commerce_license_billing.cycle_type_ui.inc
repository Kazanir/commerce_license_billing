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
    $items[$this->path]['weight'] = 100;
    // Make the listing show up as a tab.
    $items[$this->path]['type'] = MENU_LOCAL_TASK;
    // The listing should use the admin permission, not entity_access('view').
    unset($items[$this->path]['access callback']);
    $items[$this->path]['access arguments'] = array('administer licenses');
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
  $entity_info = entity_get_info('cl_billing_cycle_type');

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
  // If bundleswitcher exists, it will provide the engine form element instead.
  // If there is only one bundle, there's no point in allowing it to be changed.
  if (!module_exists('bundleswitcher') && count($entity_info['bundles']) > 1) {
    $options = array();
    foreach ($entity_info['bundles'] as $bundle => $bundle_info) {
      $options[$bundle] = $bundle_info['label'];
    }
    $form['engine'] = array(
      '#type' => 'select',
      '#title' => t('Engine'),
      '#options' => $options,
      '#default_value' => $billing_cycle_type->engine,
      '#process' => array('commerce_license_billing_cycle_type_engine_process', 'ajax_process_form'),
      '#element_validate' => array('commerce_license_billing_cycle_type_engine_validate'),
      '#ajax' => array(
        'callback' => 'commerce_license_billing_cycle_type_ajax_callback',
        'method' => 'replace',
      ),
      '#weight' => -8,
    );
  }

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
 * #process callback for the engine form element.
 */
function commerce_license_billing_cycle_type_engine_process($element, &$form_state, &$form) {
  $element['#ajax']['wrapper'] = $form['#id'];
  return $element;
}

/**
 * #element_validate callback for the engine form element.
 */
function commerce_license_billing_cycle_type_engine_validate($element, &$form_state, $form) {
  if ($element['#value'] != $element['#default_value']) {
    $form_state['rebuild'] = TRUE;
    $form_state['cl_billing_cycle_type']->engine = $element['#value'];
  }
}

/**
 * Ajax callback: returns the complete form.
 */
function commerce_license_billing_cycle_type_ajax_callback($form, $form_state) {
  return $form;
}

/**
 * Submit callback for commerce_license_billing_cycle_type_form().
 */
function commerce_license_billing_cycle_type_form_submit($form, &$form_state) {
  $billing_cycle_type = $form_state['cl_billing_cycle_type'];
  entity_form_submit_build_entity('cl_billing_cycle_type', $billing_cycle_type, $form, $form_state);
  entity_save('cl_billing_cycle_type', $billing_cycle_type);

  drupal_set_message(t('Billing cycle type %name saved.', array('%name' => $billing_cycle_type->title)));
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
