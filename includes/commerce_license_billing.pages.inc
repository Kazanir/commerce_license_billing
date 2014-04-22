<?php

/**
 * @file
 * Page callbacks for Commerce License Billing.
 */

/**
 * Page callback: Returns information about the recurring order.
 */
function commerce_license_billing_analyze_page($order) {
  if ($order->type != 'recurring') {
    return MENU_NOT_FOUND;
  }

  $wrapper = entity_metadata_wrapper('commerce_order', $order);
  $billing_cycle = $wrapper->cl_billing_cycle->value();
  $licenses = commerce_license_billing_get_recurring_order_licenses($order);
  $license_statuses = commerce_license_status_options_list();

  $cycle_start = format_date($billing_cycle->start, 'short');
  $cycle_end = format_date($billing_cycle->end, 'short');
  $header = format_string('Billing cycle #%id', array('%id' => $billing_cycle->billing_cycle_id)) . '<br />';
  $header .= format_string('From %start to %end', array('%start' => $cycle_start, '%end' => $cycle_end));

  $page = array();
  $page['#attached'] = array(
    'js' => array('misc/collapse.js','misc/form.js'),
  );
  $page['header'] = array(
    '#markup' => $header,
  );

  foreach ($licenses as $license) {
    $license_id = $license->license_id;
    $granted = format_date($license->granted, 'short');
    $status = $license_statuses[$license->status];
    $license_details = format_string('Granted on %granted', array('%granted' => $granted)) . '<br />';
    $license_details .= format_string('Status: %status', array('%status' => $status)) . '<br />';
    $license_details .= $license->accessDetails() . '<br />';
    $license_details .= commerce_license_billing_build_usage_status($license, $billing_cycle);

    $page['licenses'][$license_id] = array(
      '#type' => 'fieldset',
      '#title' => t('License #@id', array('@id' => $license_id)),
      '#attributes' => array('class' => array('collapsible')),
    );
    $page['licenses'][$license_id]['details'] = array(
      '#markup' => $license_details,
    );

    $plan_history = commerce_license_billing_plan_history_list($license, $billing_cycle);
    $items = array();
    foreach ($plan_history as $history_record) {
      $plan = commerce_product_load($history_record['product_id']);
      $start = format_date($history_record['start'], 'short');
      $end = format_date($history_record['end'], 'short');

      $item = format_string('%plan', array('%plan' => $plan->title)) . '<br />';
      $item .= format_string('From %start to %end.', array('%start' => $start, '%end' => $end));
      $item .= commerce_license_billing_build_usage_table($history_record['revision_id'], $license, $billing_cycle);
      $items[] = $item;
    }

    $page['licenses'][$license_id]['history'] = array(
      '#theme' => 'item_list',
      '#title' => t('Plan history'),
      '#items' => $items,
      '#type' => 'ol',
    );
  }

  $title = t('Order #%number', array('%number' => $order->order_number));
  $output = drupal_render($page);
  return views_megarow_display($title, $output, $order->order_id);
}

/**
 * Builds and renders the usage records status for a specific license.
 *
 * @param $license
 *   The license entity.
 * @param $billing_cycle
 *   The billing cycle entity.
 *
 * @return
 *   The status message, or an empty string if the license has no usage groups.
 */
function commerce_license_billing_build_usage_status($license, $billing_cycle) {
  if (!($license instanceof CommerceLicenseBillingUsageInterface)) {
    return '';
  }

  $incomplete = array();
  foreach ($license->usageGroups() as $group_name => $group_info) {
    $usage_group = commerce_license_billing_usage_group($license, $group_name);
    if (!$usage_group->isComplete($billing_cycle)) {
      $incomplete[] = $group_info['title'];
    }
  }

  if ($incomplete) {
    return '<b>Missing usage records for: ' . implode(', ', $incomplete) . '</b>. <br />';
  }
  else {
    return '<i> Usage records are complete. </i> <br />';
  }
}

/**
 * Builds and renders the usage records table for a specific license revision.
 *
 * @param $revision_id
 *   The revision id for which the usage table should be built.
 * @param $license
 *   The license entity.
 * @param $billing_cycle
 *   The billing cycle entity.
 *
 * @return
 *   The rendered table, or an empty string if the license has no usage groups.
 */
function commerce_license_billing_build_usage_table($revision_id, $license, $billing_cycle) {
  if (!($license instanceof CommerceLicenseBillingUsageInterface)) {
    return '';
  }

  $usage_groups = $license->usageGroups();
  $usage_history = commerce_license_billing_usage_history_list($license, $billing_cycle);
  $attributes = array('style' => 'width: 50%; margin-top: 10px;');
  $header = array(t('Usage group'), t('Quantity'), t('Start'), t('End'));
  $rows = array();
  foreach ($usage_history as $group_name => $grouped_records) {
    foreach ($grouped_records as $usage_record) {
      if ($usage_record['revision_id'] == $revision_id) {
        $row = array();
        $row[] = $usage_groups[$group_name]['title'];
        $row[] = $usage_record['quantity'];
        $row[] = format_date($usage_record['start'], 'short');
        $row[] = format_date($usage_record['end'], 'short');
        $rows[] = $row;
      }
    }
  }
  $table = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));

  // Return the table if it has at least one row.
  return $rows ? $table : '';
}
