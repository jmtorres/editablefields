<?php
// Include the files defining the classes we extend.
// This is needed in case the /cck folder lives in the main
// /modules folder (views_module_include() will then load
// content.views.inc before node.views.inc)
module_load_include('inc', 'views', 'modules/node.views');

/**
 * Implementation of hook_views_handlers().
 */
function editablefields_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'editablefields') . '/includes/views',
    ),
    'handlers' => array(
      'editablefields_handler_field_node' => array(
        'parent' => 'views_handler_field_node',
      ),
    )
  );
}

/**
 * Implementation of hook_views_data_alter().
 */
function editablefields_views_data_alter(&$data) {
  $data['node']['title']['field']['handler'] = 'editablefields_handler_field_node';
}