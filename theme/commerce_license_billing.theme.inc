<?php

/**
 * @file
 * Theme functions for Commerce License Billing.
 */

/**
 * Renders the estimation.
 */
function theme_commerce_license_billing_estimate($variables) {
  $output = array();

  $header = array(
    array('data' => t('Product'), 'style' => array('text-align: left;')),
    array('data' => t('Unit price'), 'style' => array('text-align: left;')),
    array('data' => t('Quantity'), 'style' => array('text-align: left;')),
    array(
      'data' => t('Estimation'),
    ),
  );
  $rows = array();

  foreach ($variables['estimation']['charges'] as $charge) {
    $rows[] = array(
      'data' => array(
        array(
          'data' => $charge['product']->title,
          'style' => array('text-align: left;'),
          'class' => array('product-title'),
        ),
        array(
           'data' => commerce_currency_format($charge['unit_price']['amount'], $charge['unit_price']['currency_code']),
           'class' => array('price'),
         ),
        array(
          'data' => $charge['quantity'],
          'style' => array('text-align: left;'),
        ),
        array(
          'data' => commerce_currency_format($charge['total']['amount'], $charge['total']['currency_code']),
          'class' => array('price'),
        ),
      ),
    );
  }

  // Render the table.
  $output['table'] = array(
    '#markup' => theme('table', array('header' => $header, 'rows' => $rows)),
  );

  $total = $variables['estimation']['total'];

  // Prepare an array of components for display in the footer.
  $components = array(
    'commerce-price-formatted-amount' => array(
      'title' => t('Estimated total'),
      'formatted_price' => commerce_currency_format($total['amount'], $total['currency_code']),
    ),
  );

  // Render the total area.
  $output['total'] = array(
    '#prefix' => '<div class="commerce-order-handler-area-order-total">',
    '#markup' => theme('commerce_price_formatted_components', array('components' => $components, 'price' => $total)),
    '#suffix' => '</div>',
  );

  return drupal_render($output);
}
