<?php

$plugin = array(
  'title' => t('Node editable field'),
  'entity callback' => 'node_load'
);


/*
function editablefields_node_form_alter(&$form, &$form_state, $form_id) {
  if ($form_id == 'views_ui_config_item_form') {
    if ($form_state['handler'] == 'views_handler_field_node') {
      $form['options']['editablefields'] = array(
        '#title' => t('Editable options'),
        '#type' => 'fieldset',
        'editabledfields_editable' => array(
        )
      );
    }
  }
}
*/