<?php
// $Id: views_handler_field_node.inc,v 1.7.2.2 2010/03/16 23:09:05 merlinofchaos Exp $
/**
 * @file
 * Contains the basic 'node' field handler.
 */

/**
 * Field handler to provide simple renderer that allows linking to a node.
 */
class editablefields_handler_field_node extends views_handler_field_node {
  
  /**
   * Provide link to node option
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    editablefields_views_option_form($form, $form_state, 'node');  
  }

  /**
   * Render whatever the data is as a link to the node.
   *
   * Data should be made XSS safe prior to calling this function.
   */
  function render_link($data, $values) {
    if (!empty($this->options['link_to_node'])) {
      $data = parent::render_link();
    }
    elseif(!empty($this->options['editable_allowed'])) {
      $data = 'editable link';
    }
    return $data;
  }

}
