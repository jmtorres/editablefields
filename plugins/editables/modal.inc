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
    'editablefields/%/%/%node/%' => array(
      'page callback' => 'editablefields_modal',
      'access arguments' => array('access content'),
      'page arguments' => array(1, 2, 3, 4),
      'type' => MENU_CALLBACK,
      'title' => 'Modal View',
      'file' => 'modal.inc',
    ),
  ),
  'formatter_info' => array(
    'editablefields_modal' => array(
      'label' => t('Editable Modal Widget'),
      'field types' => array_keys(_content_field_types()),
      'multiple values' => CONTENT_HANDLE_MODULE
    ),
  ),
  'build modes' => array(
    'modal' => array(
      'title' => t('Modal'),
      'views style' => FALSE,
    ),
  )
);


/**
 * Menu callback.
 *
 * @param $method 
 *  'ajax' or 'nojs'
 * @param $responder
 *  The ajax responder being used for this page request.
 * @param $node
 *  The node to be edited.
 * @param $field_name
 *  The field to be edited.
 * @return void
 */
function editablefields_modal($method, $responder = 'modal', $node, $field_name = NULL) {
  
  drupal_add_js(drupal_get_path('module', 'editablefields') . '/editablefields.js');
  ctools_include('ajax');
  ctools_include('modal');
  ctools_include('plugins');
  ctools_modal_add_js();

  $form_state = array(
    'ajax' => TRUE, 
    'node' => $node,
    '_params' => array('field_name' => $field_name, 'node_type' => $node->type)
  );

 
  $output = ctools_modal_form_wrapper('editablefields_modal_form', $form_state);

  if (!$output) {
    $output = array();
    if ($replacements = $form_state['replacements']) {
      foreach ($replacements as $nid => $text) {
        $css_id =  editablefields_modal_wrapper_id(array(
          '#type_name' => $node->type,
          '#field_name' => $field_name,
          '#node' => $node
        ));
        $output[] = ctools_ajax_command_replace(
          '#' . $css_id, '<div id="'. $css_id . '">' . $text . '</div>');
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
      
      // TODO: persist the original text formatter from original view
      $content_set_empty = content_set_empty($field, $new_values);
      foreach ((array)$node->{$field['field_name']} as $delta => $item) {
        $item['delta'] = $delta;
        $replacements[$nid] .= editablefields_modal_link_text($field, $item, $field['display_settings']['modal']['format'], $node);
      }
    }
  }
  
  $form_state += array(
    'replacements' => $replacements,
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
 * Default theme implementation for editablefields_modal.
 */
function theme_editablefields_modal($element) {
  static $fields = array();
  static $views_field_edit_column = array();
  $node = $element['#node'];
  drupal_add_js(drupal_get_path('module', 'editablefields') . '/editablefields.js');
  ctools_include('ajax');
  ctools_include('modal');
  ctools_modal_add_js();
  
  /**
   * Keep track of the current Views row/column so we can apply 
   * Views specified formatter settings.
   */
  if (!empty($node->field_edit)) {
    $view = views_get_current_view();
    if (empty($views_field_edit_column[$view->name][$view->row_index])) {
      $views_field_edit_column[$view->name][$view->row_index] = 0;
    }
    $views_options = $node->field_edit[$views_field_edit_column[$view->name][$view->row_index]];
    $views_field_edit_column[$view->name][$view->row_index]++;
  }
    
  $type = $element['#type_name'];
  $field_name = $element['#field_name'];
  $context = !empty($node->content) && !empty($node->content[$field_name]) ? $node->content[$field_name]['#context'] : 'full';
  
  if (empty($fields[$type][$field_name])) {
    $fields[$type][$field_name] = content_fields($field_name, $type);
  }
  
  $field = $fields[$type][$field_name];
  $options = editablefields_formatter_get_settings($field_name, $type, $context);
  if (!empty($views_options)) {
    $options = $views_options;
  }
  
  // dpm($field);
  $text_formatter = ($options['text_formatter']) ? $options['text_formatter'] : $field['display_settings']['modal']['format'];
  $css_id = editablefields_modal_wrapper_id($element);
  foreach ($element as $delta => $item) {
    if (is_integer($delta)) {
      $link_text .= editablefields_modal_link_text($field, $item['#item'], $text_formatter, $node);
    }
  }
  
  $link_text = '<div id="' . $css_id . '">' . $link_text . '</div>';
  $output =  editablefields_modal_text_button($link_text, 'editablefields/nojs/modal/' . $element['#node']->nid . '/' . $element['#field_name'] , check_plain($link_text));
  ctools_include('plugins');
  return $output;
}

function editablefields_modal_link_text($field, $item, $formatter_name, $node) {
  // Other module's formatters might be doing something just as exotic
  // as we are but we can only allow tags that won't mess with our button.
  // We use filter_xss just for its html stripping magic.
  $allowed_tags = array('img', 'div', 'span', 'em', 'strong', 'cite', 'ul', 'ol', 'li', 'dl', 'dt', 'dd'); 
  $output = filter_xss(content_format($field, $item, $formatter_name, $node), $allowed_tags);
  
  if ($output) {
    return $output;
  }
  else {
    return t('Not set');
  }
}

/**
 * Render text as a link. This will automatically apply an AJAX class
 * to the link and add the appropriate javascript to make this happen.
 *
 * Note: 'html' => true so be sure any text is vetted! Chances are these kinds of buttons will
 * not use user input so this is a very minor concern.
 *
 * @param $image
 *   The path to an image to use that will be sent to theme('image') for rendering.
 * @param $dest
 *   The destination of the link.
 * @param $alt
 *   The alt text of the link.
 * @param $class
 *   Any class to apply to the link. @todo this should be a options array.
 */
function editablefields_modal_text_button($text, $dest, $alt, $class = '') {
  return ctools_ajax_text_button($text, $dest, $alt, $class, 'editablefields-use-ctools-modal');
}

function editablefields_modal_wrapper_id($element) {
  return 'editablefields-modal-' . $element['#type_name'] . '-' . $element['#field_name'] . '-' . $element['#node']->nid;
}




