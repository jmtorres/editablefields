<?php

/**
 * @file
 * Defines CTools Export UI plugin for Editablefields..
 */

/**
 * Define this Export UI plugin.
 */
$plugin = array(
  // As defined in hook_schema().
  'schema' => 'editablefields_preset',
  // Define a permission users must have to access these pages.
  'access' => 'administer editablefields',

  // Define the menu item.
  'menu' => array(
    'menu item' => 'editablefields',
    'menu title' => 'Editablefields',
    'menu description' => 'Administer Editablefields presets.',
  ),

  // Define user interface texts.
  'title singular' => t('preset'),
  'title plural' => t('presets'),
  'title singular proper' => t('Editablefields preset'),
  'title plural proper' => t('Editablefields presets'),

  // Define the names of the functions that provide the add/edit forms.
  'form' => array(
    'settings' => 'editablefields_ctools_export_ui_form',
    // 'submit' and 'validate' are also valid callbacks.
  ),
);


/**
 * Define the preset add/edit form.
 */
function editablefields_ctools_export_ui_form(&$form, &$form_state) {
  $preset = $form_state['item'];

  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#description' => t('The human readable name or description of this preset.'),
    '#default_value' => $preset->description,
    '#required' => TRUE,
  );

  // Add Mymodule's configuration interface.
  $form['config'] = array(
    '#type' => 'textfield',
    '#title' => t('Editablefields configuration'),
    '#description' => t('This is just the simplest possible example of a configuration interface.'),
    '#default_value' => $preset->config,
    '#required' => TRUE,
  );

}
