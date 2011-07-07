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
  'title' => t('Editable Modal Widget'),
  'hook_menu' => array(
    'editablefields/%ctools_js/modal/%/%editablefields_object/%' => array(
      'page callback' => 'editablefields_modal',
      'access arguments' => array('access content'),
      'page arguments' => array(1, 2, 3, 4, 5),
      'load arguments' => array(3),
      'type' => MENU_CALLBACK,
      'title' => 'Modal View',
      'file' => 'modal.inc',
    ),
  ),
  'formatter_info' => array(
    'editablefields_modal' => array(
      'label' => t('Editable Modal Widget'),
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
 * RESPONDER 
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
function editablefields_modal($method, $responder = 'modal', $editable, $object, $field_name = NULL) {
  
  drupal_add_js(drupal_get_path('module', 'editablefields') . '/editablefields.js');
  ctools_include('ajax');
  ctools_include('modal');
  ctools_include('plugins');
  ctools_modal_add_js();

  $form_state = array(
    'ajax' => TRUE, 
    'node' => $object,
    '_params' => array('field_name' => $field_name, 'node_type' => $object->type)
  );

 
  $output = ctools_modal_form_wrapper('editablefields_' . $editable . '_form', $form_state);

  if (!$output) {
    $output = array();
    if ($replacements = $form_state['replacements']) {
      foreach ($replacements as $nid => $replacement) {
        $css_id =  editablefields_modal_wrapper_id(array(
          '#type_name' => $object->type,
          '#field_name' => $field_name,
          '#node' => $object
        ));
        
        $function = 'editablefields_' . $editable . '_render';
        $html = $function($replacement['field'], $replacement['item'], $replacement['object'], 'modal');
        
        $output[] = ctools_ajax_command_replace(
          '#' . $css_id, '<div id="'. $css_id . '">' . $html . '</div>');
      }
    }
    $output[] = ctools_modal_command_dismiss();
  }
  ctools_ajax_render($output);
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
 * RESPONDER
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

/**
 * RESPONDER
 */
function editablefields_modal_wrapper_id($element) {
  return 'editablefields-modal-' . $element['#type_name'] . '-' . $element['#field_name'] . '-' . $element['#node']->nid;
}





