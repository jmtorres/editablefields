<?php

$plugin = array(
  'title' => t('CCK editable field'),
  'formatter_info' => array(
    'field types' => array_keys(_content_field_types()),
    'multiple values' => CONTENT_HANDLE_MODULE
  ),
  'entity callback' => 'node_load',
  'views handlers' => array(
    'content_handler' => array(
      'fields' => array(),
    )
  ),
);

/**
 * EDITABLE
 * Form API form function for field_edit_form.
 * Returns a form api definition for editing fields.
 */
function editablefields_cck_form(&$form_state) {
  $form = array();
  $params = empty($form_state['_params']) ? array() : $form_state['_params'];
  module_load_include('inc', 'content', 'includes/content.node_form');

  $form['#node'] = $form_state['node'];
  $node = $form_state['node'];
  $nids = array($node->nid);
  $field = content_fields($params['field_name'], $params['node_type']);
  $css_class = 'form-' . str_replace('_', '-', $params['field_name']);
  $form['#attributes'] = array('class' => $css_class);

  $form['#field_info'][$params['field_name']] = $field;
  $form += (array)content_field_form($form, $form_state, $field);
  $form[$params['field_name']]['#weight'] = 0;

  $form['nids'] =
    array('#type' => 'hidden', '#default_value' => implode(',', $nids));

  $form['submit'] =
    array('#type' => 'submit', '#value' => t('Submit'), '#weight' => 20);

  return $form;
}

/**
 * Submit handler for fields.
 * EDITABLE
 */
function editablefields_cck_form_submit($form, &$form_state) {
  $nids = explode(',', trim($form_state['values']['nids'], ','));
  $nids = array_unique($nids);
  $replacements = $additions = $deletions = array();
  $field = $form['#field_info'][$form_state['_params']['field_name']];

  foreach ($nids as $nid) {
    if ($node = node_load($nid, NULL, TRUE)) {
      $old_values = $node->{$field['field_name']};
      $new_values = $form_state['values'][$field['field_name']];

      $node->{$field['field_name']} = $new_values;
      $node = node_submit($node);
      node_save($node);
      
      // TODO: persist the original text formatter from original view
      $content_set_empty = content_set_empty($field, $new_values);
      foreach ((array)$node->{$field['field_name']} as $delta => $item) {
        $item['delta'] = $delta;
        // TODO: need to make this generic for all responders
        // Probably just pass an array.
        // $replacements[$nid] .= editablefields_modal_link_text($field, $item, $field['display_settings']['modal']['format'], $node);
        $replacements[$nid][] = array(
          'field'   => $field,
          'item'    => $item,
          'object'  => $node
        );
      }
    }
  }
  
  $form_state += array(
    'replacements' => $replacements,
  );
}


