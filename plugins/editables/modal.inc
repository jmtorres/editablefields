<?php

/**
 * @file
 * Editablefields CTools modal plugin.
 *
 * Provides a plugin that shows the editable fields in the default CTools
 * modal dialog.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t('Editable Modal Widget'),
  'hook_menu' => array(
    'editablefields/modal/%node/%' => array(
      'page callback' => 'editablefields_modal',
      'access arguments' => array('access content'),
      'page arguments' => array(2, 3),
      'type' => MENU_CALLBACK,
      'title' => 'Modal View',
//      'file' => 'editablefields.modal.inc',
    ),
  ),
  'formatter_info' => array(
    'editablefields_modal' => array(
      'label' => t('Editable Modal Widget'),
      'field types' => array('datetime', 'text'),
      'multiple values' => CONTENT_HANDLE_CORE
    ),
  ),
);


/**
 * Menu callback.
 *
 * @param  $node
 * @param null $fields
 * @return void
 */
function editablefields_modal($node, $fields = NULL) {
  ctools_include('ajax');
  ctools_include('modal');

  $form_state =
    array('ajax' => TRUE, 'node' => $node,
      '_params' => array('field_name' => $fields, 'node_type' => $node->type));

  $form_id = 'editablefields_modal_' . $node->type . '_' . $fields;
  $form_id_css = str_replace('_', '-', $form_id);

  $output = ctools_modal_form_wrapper('editablefields_modal_form', $form_state);
  ctools_include('plugins');

  if (!$output) {
    $output = array();
    $output[] = ctools_ajax_command_replace('#modal-date-message-' . $node->nid,
                                            '<div id="modal-date-message-'
                                              . $node->nid . '">' .
                                              $form_state['message']
                                              . '</div>');
    if ($replacements = $form_state['replacements']) {
      foreach ($replacements as $nid_delta => $text) {
        $css_id = $form_id_css . '-' . $nid_delta;
        $output[] = ctools_ajax_command_replace(
          '#' . $css_id . ' a > span', '<span>' . $text . '</span>');
      }
    }
    $last_css_id = $css_id;
    if ($additions = array_reverse($form_state['additions'])) {
      foreach ($additions as $nid_delta => $text) {
        $css_id = $form_id_css . '-' . $nid_delta;
        $text = '<div id="' . $css_id . '">' . ctools_modal_text_button($text,
                                                                        'editablefields_modal/'
                                                                          . $node->nid
                                                                          . '/'
                                                                          . $fields,
                                                                        $text)
          . '</div>';
        $output[] = ctools_ajax_command_after('#' . $last_css_id, $text);
      }
    }
    if ($deletions = $form_state['deletions']) {
      foreach ($deletions as $nid_delta => $remove) {
        $output[] = ctools_ajax_command_remove(
          '#' . $form_id_css . '-' . $nid_delta);
      }
    }
    $output[] = ctools_modal_command_dismiss();
  }
  ctools_ajax_render($output);
}


/**
 * Form API form function for field_edit_form.
 * Returns a form api definition for editing fields.
 */
function editablefields_modal_form(&$form_state) {
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
 */
function editablefields_modal_form_submit($form, &$form_state) {
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
      // $field['display_settings']['label']['format'] = 'hidden';
      // TODO: persist the original text formatter from original view
      $old_values_count = count($old_values);
      $new_values_count = count(_editablefields_filter_field_post_values($field,
                                                                         $new_values));

      $content_set_empty = content_set_empty($field, $new_values);
      $formatter =
        $field['widget']['module'] . '_formatter_' . $field['default_format'];
      foreach ((array)$node->{$field['field_name']} as $delta => $item) {
        $item['#delta'] = $delta;
        if ($delta < $old_values_count) {
          $replacements["{$nid}-{$delta}"] = content_format($field, $item,
                                                            'default', $node);
        }
        else {
          $additions["{$nid}-{$delta}"] = content_format($field, $item,
                                                         'default', $node);
        }
      }

      if ($old_values_count > $new_values_count) {
        $limit = $new_values_count;
        for ($delta = $limit; $delta < $old_values_count; $delta++) {
          $deletions["{$nid}-{$delta}"] = TRUE;
        }

      }

    }
  }
  $form_state['redirect'][0] = $_GET['q'];
  $form_state['redirect'][1] = drupal_query_string_encode($_GET, array('q'));
  $form_state['message'] = t('Reservation has been updated');
  $form_state += array(
    'replacements' => $replacements,
    'additions' => $additions,
    'deletions' => $deletions,
  );
}

/**
 * $values is the POSTed values array from the browser. With multivalued fields
 * it will also contain the "Add more items" string which we don't need.
 *
 * @param  $field
 *   The CCK field array.
 * @param  $values
 *   The values POSTed from the browser.
 * @return array
 *   Return the filtered array of non-empty values.
 */
function _editablefields_filter_field_post_values($field, $values) {
  return array_filter((array)content_set_empty($field, $values), 'is_array');
}


/**
 * Default theme implementation for field_edit_widget.
 * TODO: turn this into an render api helper fuction.
 */
function theme_editablefields_modal($element) {
  static $fields = array();
  static $used_aliases = array();
  drupal_add_js(drupal_get_path('module', 'field_edit') . '/js/field_edit.js');
  ctools_include('ajax');
  ctools_include('modal');
  ctools_modal_add_js();
  $node = $element['#node'];
  $type = $element['#type_name'];
  $field_name = $element['#field_name'];
  $context =
    !empty($node->content) && !empty($node->content[$field_name]) ?
      $node->content[$field_name]['#context'] : 'full';

  if (empty($fields[$type][$field_name])) {
    $fields[$type][$field_name] = content_fields($field_name, $type);
  }

  $field = $fields[$type][$field_name];
  $options = editablefields_formatter_get_settings($field_name, $type,
                                                   $context);
  if (!empty($node->field_edit) && !empty($node->field_edit[$field_name])) {
    foreach ($node->field_edit[$field_name] as $alias => $format_options) {
      if (!in_array($alias, $used_aliases)) {
        $options = $format_options;
      }
    }
  }

  $text_formatter =
    ($options['text_formatter']) ? $options['text_formatter'] : 'default';
  $display_data = content_format($field, $element['#item'], $text_formatter,
                                 $node);

  if ($display_data == '') {
    $display_data = '<span>' . t('Not Set') . '</span>';
  }
  else {
    $display_data = '<span>' . $display_data . '</span>';
  }

  $form_id = 'editablefields_modal_' . $element['#type_name'] . '_' .
    $element['#field_name'];
  $css_id =
    str_replace('_', '-', $form_id) . '-' . $element['#node']->nid . '-' .
      $element['#item']['#delta'];
  $form_url = 'editablefields/modal/' .$element['#node']->nid . '/' .
    $element['#field_name'];
  $output =
    '<div id="' . $css_id . '">' .
      ctools_modal_text_button($display_data, $form_url, $display_data)
      . '</div>';
  ctools_include('plugins');
  return $output;

}
