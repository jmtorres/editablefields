<?php

$plugin = array(
  'title' => t('CCK editable field'),
  'theme' => array(
    'editablefields_formatter_modal' => array(
      'arguments' => array('element' => NULL),
      'function' => 'theme_editablefields_formatter',
      'file' => drupal_get_path('module', 'editablefields') . '/plugins/editables/cck.inc'
    ),
  ),
  'formatter_info' => array(
    'modal' => array(
      'label' => t('Modal Widget'),
      'field types' => array_keys(_content_field_types()),
      'multiple values' => CONTENT_HANDLE_MODULE
    )
  ),
  'views handlers' => array(
    'content_handler' => array(
      'fields' => array(),
    )
  ),
  'entity callback' => 'node_load'
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

      // TODO: what's this for?  do we need it
      $content_set_empty = content_set_empty($field, $new_values);
      
      $replacements[$nid] = array(
        'field'   => $field,
        'item'    => $node->{$field['field_name']},
        'object'  => $node
      );
      
    }
  }
  
  $form_state += array(
    'replacements' => $replacements,
  );
}

/**
 * Default theme implementation for editablefields_modal.
 * RESPONDER
 */
function theme_editablefields_formatter($element) {

  static $fields = array();
  static $views_field_edit_column = array();
  $responder = 'modal';
  
  $node = $element['#node'];
  // ajax-responder.js needs to be before editablefields.js
  require_once drupal_get_path('module', 'editablefields') . '/plugins/responders/' . $responder . '.inc';
  ctools_add_js('ajax-responder');
  drupal_add_js(drupal_get_path('module', 'editablefields') . '/editablefields.js');
  ctools_include('plugins');
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
  
  $text_formatter = !empty($options['text_formatter']) ? $options['text_formatter'] : $field['display_settings']['modal']['format'];
  
  $function = 'editablefields_' . $responder . '_wrapper_id';
  $css_id = $function($element);
  $link_text = '';
  
  foreach ($element as $delta => $item) {
    if (is_integer($delta)) {
      $items[$delta] = $item['#item'];
    }
  }
  
  $link_text = editablefields_cck_render($field, $items, $node, $responder);
  $link_text = '<div id="' . $css_id . '">' . $link_text . '</div>';
  
  $function = 'editablefields_' . $responder . '_text_button';
  $output =  $function($link_text, 'editablefields/nojs/modal/cck/' . $element['#node']->nid . '/' . $element['#field_name'] , check_plain($link_text));

  return $output;
}


function editablefields_cck_render($field, $items, $node, $responder) {
  require_once drupal_get_path('module', 'content') .'/includes/views/content.views.inc';
  $formatter_name = $field['display_settings'][$responder]['format'];
  // Other module's formatters might be doing something just as exotic
  // as we are but we can only allow tags that won't mess with our button.
  // We use filter_xss just for its html stripping magic.
    
  // Some formatters need to behave differently depending on the build_mode
  // (for instance: preview), so we provide one.
  $node->build_mode = NODE_BUILD_NORMAL;
  foreach ($items as $delta => $item) {
    $items[$delta]['#delta'] = $delta;
  }

  // Render items.
  $allowed_tags = array('img', 'div', 'span', 'em', 'strong', 'cite', 'ul', 'ol', 'li', 'dl', 'dt', 'dd');
  if ($items && ($formatter = _content_get_formatter($formatter_name, $field['type']))) {
    $rendered = array();
    if (content_handle('formatter', 'multiple values', $formatter) == CONTENT_HANDLE_CORE) {
      // Single-value formatter.
      foreach ($items as $item) {
        $rendered[] = filter_xss(content_format($field, $item, $formatter_name, $node), $allowed_tags);
      }
    }
    else {
      // Multiple values formatter.
      $rendered[] = filter_xss(content_format($field, $items, $formatter_name, $node), $allowed_tags);
    }

    if (count($rendered) > 1) {
      // TODO: could we use generic field display ?
      $output = theme('content_view_multiple_field', $rendered, $field, $items);
    }
    elseif ($rendered) {
      $output = $rendered[0];
    }

  }
  if ($output) {
    return $output;
  }
  else {
    // TODO: "Not set" is not necessarily true.  For instance, CCK returns
    // an empty string for files that are not "listed".     
    return t('Not set');
  }
}




