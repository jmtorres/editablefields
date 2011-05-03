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
$plugin = array(
  'title' => t('Editable Inline Widget'),
  'hook_menu' => array(
    'editablefields/%/inline/%node/%' => array(
      'page callback' => 'editablefields_inline',
      'access arguments' => array('access content'),
      'page arguments' => array(1, 2, 3, 4),
      'type' => MENU_CALLBACK,
      'title' => t('Inline Widget'),
      'file' => 'inline.inc',
    ),
  ),
  'formatter_info' => array(
    'editablefields_inline' => array(
      'label' => t('Editable Inline Widget'),
      'field types' => array_keys(_content_field_types()),
      'multiple values' => CONTENT_HANDLE_MODULE
    ),
  ),
  'build modes' => array(
    'inline' => array(
      'title' => t('Inline'),
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
function editablefields_inline($method, $responder = 'inline', $node, $field_name = NULL) {
  
  drupal_add_js(drupal_get_path('module', 'editablefields') . '/editablefields.js');
  ctools_include('ajax');
  ctools_include('plugins');

  $form_state = array(
    'ajax' => TRUE, 
    'node' => $node,
    '_params' => array('field_name' => $field_name, 'node_type' => $node->type)
  );

  $output = editablefields_inline_form_wrapper('editablefields_' . $responder . '_form', $form_state);

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
    // $output[] = ctools_modal_command_dismiss();
  }
  ctools_ajax_render($output);
}


/**
 * Form API form function for field_edit_form.
 * Returns a form api definition for editing fields.
 */
function editablefields_inline_form(&$form_state) {
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
function editablefields_inline_form_submit($form, &$form_state) {
  $nids = explode(',', trim($form_state['values']['nids'], ','));
  $nids = array_unique($nids);
  $replacements = array();
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
        $replacements[$nid] .= editablefields_inline_link_text($field, $item, $field['display_settings']['modal']['format'], $node);
      }
    }
  }
  
  $form_state += array(
    'replacements' => $replacements,
  );
}

/**
 * Default theme implementation for editablefields_modal.
 */
function theme_editablefields_inline($element) {
  static $fields = array();
  static $views_field_edit_column = array();
  $node = $element['#node'];
  drupal_add_js(drupal_get_path('module', 'editablefields') . '/editablefields.js');
  ctools_include('ajax');
  // ctools_modal_add_js();
  
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
  
  $text_formatter = ($options['text_formatter']) ? $options['text_formatter'] : $field['display_settings']['modal']['format'];
  $css_id = editablefields_inline_wrapper_id($element);
  foreach ($element as $delta => $item) {
    if (is_integer($delta)) {
      $link_text .= editablefields_inline_link_text($field, $item['#item'], $text_formatter, $node);
    }
  }
  
  $link_text = '<div id="' . $css_id . '">' . $link_text . '</div>';
  $output =  editablefields_inline_text_button($link_text, 'editablefields/nojs/inline/' . $element['#node']->nid . '/' . $element['#field_name'] , check_plain($link_text));
  ctools_include('plugins');
  return $output;
}

function editablefields_inline_link_text($field, $item, $formatter_name, $node) {
  // Other module's formatters might be doing something just as exotic
  // as we are but we can only allow tags that won't mess with our button.
  // We use filter_xss just for its html stripping magic.
  $allowed_tags = array('img', 'div', 'span', 'em', 'strong', 'cite', 'ul', 'ol', 'li', 'dl', 'dt', 'dd'); 
  $output = filter_xss(content_format($field, $item, $formatter_name, $node), $allowed_tags);
  
  if ($output) {
    return $output;
  }
  else {
    // TODO: "Not set" is not necessarily true.  For instance, CCK returns
    // an empty string for files that are not "listed".     
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
function editablefields_inline_text_button($text, $dest, $alt, $class = '') {
  return ctools_ajax_text_button($text, $dest, $alt, $class, 'editablefields-use-inline');
}

function editablefields_inline_wrapper_id($element) {
  return 'editablefields-inline-' . $element['#type_name'] . '-' . $element['#field_name'] . '-' . $element['#node']->nid;
}

function editablefields_inline_form_wrapper($form_id, &$form_state) {
  ctools_include('form');
  // This won't override settings already in.
  $form_state += array(
    're_render' => FALSE,
    'no_redirect' => !empty($form_state['ajax']),
  );

  $output = ctools_build_form($form_id, $form_state);
  if (!empty($form_state['ajax']) && empty($form_state['executed'])) {
    return editablefields_inline_form_render($form_state, $output);
  }

  return $output;
}

/**
 * Render a form into an AJAX display.
 */
function editablefields_inline_form_render($form_state, $output) {
  $title = empty($form_state['title']) ? drupal_get_title() : $form_state['title'];

  // If there are messages for the form, render them.
  if ($messages = theme('status_messages')) {
    $output = '<div class="messages">' . $messages . '</div>' . $output;
  }

  $commands = array();
  if (isset($form_state['js settings'])) {
    $commands[] = ctools_ajax_command_settings($form_state['js settings']);
  }

  //$commands[] = editablefields_inline_command_display($title, $output);
  $commands[] = array(
    'command' => 'replace',
    'selector' => '#editablefields-inline-test-field_test_file-6',
    'data' => $output
  );
  return $commands;
}

function editablefields_inline_command_display($title, $html) {
  return array(
    'command' => 'alert',
    'title' => $title,
    'data' => $html,
  );
}







