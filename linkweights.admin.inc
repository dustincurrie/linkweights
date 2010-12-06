<?php
// $Id: linkweights.admin.inc,v 1.3 2010/12/06 04:16:16 dustincurrie Exp $

/**
 * @file
 * Administrative page callbacks for the linkweights module.
 */

/**
 * Callback; display list of content types and link editing form.
 *
 * @return $output
 */
function linkweights_settings_overview() {
  /**
   * In some cases function module_invoke_all('node_info') DOES NOT return all custom node types
   * function node_get_types() used in ths case. It returns absolutly all.
   * Also added some usefull info columns in overview table:)
   */
  $node_types_info = node_get_types();
  foreach ($node_types_info as $node_type) {
    $rows[] = array(
      $node_type->type,
      $node_type->module,
      $node_type->description,
      l(t('edit'), 'admin/settings/linkweights/'. $node_type->type),
    );
  }
  return theme('table', array(t('Node Type'), t('Module'), t('Description'), t('Configure Links')), $rows);
}

/**
 * Form builder; Modify order of link diplay.
 *
 * @ingroup forms
 * @see drupal_get_form()
 */
function linkweights_settings_form($form_state, $linkweights_nodetype) {
  $sample = db_fetch_object(db_query_range("SELECT nid FROM {node} WHERE type='%s'", $linkweights_nodetype, 0, 1));
  $links = module_invoke_all('link', 'node', node_load($sample->nid));
  $form['nodetype'] = array('#type' => 'hidden', '#value' => $linkweights_nodetype);
  $form['#tree'] = TRUE;
  $form['links'] = array(
    '#type' => 'fieldset',
    '#title' => t('Available Links'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );
  $results = db_query("SELECT * FROM {linkweights} WHERE nodetype='%s'", $linkweights_nodetype);
  while ($row = db_fetch_array($results)) {
    $stored_weight[$row['link']] = $row['weight'];
  }
  foreach ($links as $linkname => $link) {
    $form['links'][$linkname]['weight']  = array(
      '#type' => 'weight',
      '#delta' => 50,
      '#default_value' => (isset($stored_weight[$linkname])) ? $stored_weight[$linkname] : 0,
      '#title' => strip_tags($link['title']),
    );
    $form['links'][$linkname]['weight']['#attributes']['class'] = 'linkweights-weight';
  }
  uasort($form['links'], '_linkweights_link_cmp');
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  return $form;
}

function linkweights_theme() {
  return array(
    'linkweights_settings_form' => array(
      'arguments' => array('form' => NULL),
    ),
  );

}

/**
 * Implementation of hook_submit().
 * @param $form
 * @param $form_state
 * @return unknown_type
 */
function linkweights_settings_form_submit($form, &$form_state) {
  db_query("DELETE FROM {linkweights} WHERE nodetype='%s'", $form_state['values']['nodetype']);
  foreach (element_children($form_state['values']['links']) as $val) {
    db_query("INSERT INTO {linkweights} (nodetype, link, weight) VALUES('%s', '%s', %d)", $form_state['values']['nodetype'], $val, $form_state['values']['links'][$val]['weight']);
  }
  // Built-in widely used string
  drupal_set_message(t('Configuration saved.'));
}

/**
 * Theme the links by nodetype as a sortable list of terms.
 * @ingroup themeable
 * @see linkweights_settings_form()
 */

function theme_linkweights_settings_form($form) {
  drupal_add_tabledrag('node-link-weights-table', 'order', 'sibling', 'node-links-weight');
  foreach (element_children($form['links']) as $key) {
      $row = array();
      $row[] = $key;
      $form['links'][$key]['weight']['#attributes']['class'] = 'node-links-weight';
      $row[] = drupal_render($form['links'][$key]['weight']);
      $rows[] = array('data' => $row, 'class' => 'draggable');
  }
  if (empty($rows)) {
    $rows[] = array(array('data' => t('No links are available for this content type')));
  }
  /**
   * Minor fix of table.
   * To see bug you need edit nodetype with no links
   */
  $header = array('data' => t('Link'), 'attributes' => array('colspan' => '2'));
  $form['links']['markup'] = array('#value' => theme('table', $header, $rows, array('id' => 'node-link-weights-table')));
  $output .= drupal_render($form);
  return $output;
}