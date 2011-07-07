
/**
 * We use event delegation extensively for performance reasons.
 * $.live() function can achieve the same effect but is not
 * as efficient as this implementation. This also takes care of any additional DOM
 * changes that might happen in the page (ie. AJAX, AHAH, etc)
 *
 * Further reading about event delegation:
 *   http://blogs.sitepoint.com/2008/07/23/javascript-event-delegation-is-easier-than-you-think/
 *   http://developer.yahoo.com/yui/examples/event/event-delegation.html
 *   http://icant.co.uk/sandbox/eventdelegation/
 *
 **/

// @todo: Can only have one active clicktoedit datepicker field.
// @todo: (related to above todo) clicktoedit datepicker fields load twice after changing date.
Drupal.behaviors.editablefields = function(context) {
  
  // load the ajax-editable fields
  $('div.editablefields.ajax-editable', context).not('.editablefields-processed').each(function() {
    $(this).addClass('editablefields-processed');
    Drupal.editablefields.load(this);
  });
  
  // console.log($('form#editablefields-inline-form:not(.editablefields-use-inline-processed'));
    // Bind submit links in the inline form.
  $('#editablefields-inline-form:not(.editablefields-use-inline-processed)')
    .addClass('editablefields-use-inline-processed')
    .submit(Drupal.editablefields.inline.submitAjaxForm)
    .bind('CToolsAJAXSubmit', Drupal.CTools.AJAX.ajaxSubmit);
    
  // We are not taking 'context' into consideration on purpose here
  // in order to add event handlers only once per page.
  if (!$('body').hasClass('editablefields-processed')) {
    $('body').addClass('editablefields-processed');

    // bind a global document event handler
    $(document).bind('change click', function(event) {
      if (event.type == 'change') {
        if ($(event.target).is('input, textarea, select')) {
          if ($(event.target).parents('.editablefields').not('.ajax-editable').length) {
            Drupal.editablefields.onchange($(event.target).parents('.editablefields'));
          }
        }
      }
      else if (event.type == 'click') {
        if ($(event.target).not('.ajax-editable').is('.editablefields')) {
          Drupal.editablefields.init.call($(event.target));
        }
        else if ($(event.target).parent('.editablefields').not('.ajax-editable').length) {
          Drupal.editablefields.init.call($(event.target).parent('.editablefields'));
        }
        
        //TODO: plugin classes and event callbacks should be in js settings
        $(event.target).filter('.editablefields-use-ctools-modal *').each(function() {  
          var link = $(this).is('a') ? this : $(this).parents('a.editablefields-use-ctools-modal')[0];
          if (typeof(link) != "undefined") {
            Drupal.CTools.Modal.clickAjaxLink.apply(link);
            event.preventDefault();
          }
        });
        
        $(event.target).filter('.editablefields-use-inline *').each(function() {  
          var link = $(this).is('a') ? this : $(this).parents('a.editablefields-use-inline')[0];
          if (typeof(link) != "undefined") {
            Drupal.editablefields.inline.clickAjaxLink.apply(link);
            //console.log('inline');
            event.preventDefault();
          }
        });
      }
    });
    
    // prevent form submits for editable textfields when the Enter key is hit
    // @see: http://drupal.org/node/1002582
    $(document).keydown(function(event) {
      if (event.keyCode == '13' && $(event.target).is(':text') &&
           $(event.target).parents('.editablefields').not('.ajax-editable').length) {
        event.preventDefault();
      }
    });
  }
};


// Initialize editablefields object.
Drupal.editablefields = {};

// Initialize array where all the clicked/active objects live
// Each time an editable field link is clicked or active (think modal)
// it is inserted in this array. The index where it was inserted is sent to the
// server and is returned so that JS code knows what to replace
Drupal.editablefields.active_elements = [];

// Create a unique index for checkboxes
Drupal.editablefields.checkbox_fix_index = 0;

Drupal.editablefields.inline = {};
Drupal.editablefields.inline.clickAjaxLink = function() {
  if ($(this).hasClass('ctools-ajaxing')) {
    return false;
  }

  var url = $(this).attr('href'),
    // we are adding an element (see below) so the index is the same
    // as the array length
    active_elem_index = Drupal.editablefields.active_elements.length;
  // insert the clicked object in the active_elements array
  // active_elem_index's value will be sent to the server so that
  // we can keep track of what element to replace
  Drupal.editablefields.active_elements.push($(this));

  // mark the element as ajax active
  $(this).addClass('ctools-ajaxing');
  try {
    url = url.replace(/\/nojs(\/|$)/g, '/ajax$1');
    $.ajax({
      type: "POST",
      url: url,
      data: {'js': 1, 'ctools_ajax': 1, 'active_elem_index': active_elem_index},
      global: true,
      success: Drupal.CTools.AJAX.respond,
      error: function(xhr) {
        Drupal.CTools.AJAX.handleErrors(xhr, url);
      },
      complete: function() {
        $('.ctools-ajaxing').removeClass('ctools-ajaxing');
      },
      dataType: 'json'
    });
  }
  catch (err) {
    alert("An error occurred while attempting to process " + url + ': ' + err);
    $('.ctools-ajaxing').removeClass('ctools-ajaxing');
    return false;
  }

  return false;
};

Drupal.editablefields.inline.submitAjaxForm = function(e) {
  var url = $(this).attr('action');
  var form = $(this);
 
  setTimeout(function() { Drupal.CTools.AJAX.ajaxSubmit(form, url); }, 1);
  return false;
};


Drupal.editablefields.init = function() {
  $(this).unbind("click");
  $(this).parents('div.field').find('.field-label, .field-label-inline-first, .field-label-inline, .field-label-inline-last').addClass('highlighted');
  $(this).addClass('editablefields-processed');
  $(this).children().hide();
  Drupal.editablefields.load(this);
};

Drupal.editablefields.view = function(element) {
  if ($(element).hasClass("editablefields_REMOVE") ) {
    $(element).hide();
  }
  else {
    $(element).addClass('editablefields_throbber');

    var url = Drupal.settings.editablefields.url_view + "/" + $(element).attr("nid") + "/" + $(element).attr("field")+ "/" + $(element).attr("delta");
    $.ajax({
      url: url,
      type: 'GET',
      success: function(response) {
        // Call all callbacks.
        if (response.__callbacks) {
          $.each(response.__callbacks, function(i, callback) {
            eval(callback)(element, response);
          });
        }
        $(element).html(response.content);
        Drupal.attachBehaviors(element);
        var len = response.content.length;

        // there is not way for the server to know which formatter we are using for this field as the view is not
        // available during this request so we add the message with JS instead.
        if(len) {
          $(element).prepend(Drupal.settings.editablefields.clicktoedit_message);
        }
        else {
          $(element).prepend(Drupal.settings.editablefields.clicktoedit_message_empty);
        }

        $(element).bind("click",Drupal.editablefields.init);
        $(element).removeClass('editablefields_throbber');
        $(element).removeClass('editablefields-processed');
      },
      error: function(response) {
        $(".messages.error").remove();
        $(element).after('<div class="messages error">' + Drupal.t("An error occurred at ") + url + '</div>');
        $(".messages.error").hide(0).show(1000);
        $(element).removeClass('editablefields_throbber');
        $(element).removeClass('editablefields-processed');
      },
      dataType: 'json'
    });
  }
};

Drupal.editablefields.load = function(element) {
  $(".content .messages.status").remove();
  if ($(element).hasClass("editablefields_REMOVE") ) {
    $(element).hide();
  }
  else {
    $(element).addClass('editablefields_throbber');

    var url = Drupal.settings.editablefields.url_html + "/" + $(element).attr("nid") + "/" + $(element).attr("field")+ "/" + $(element).attr("delta");
    $.ajax({
      url: url,
      type: 'GET',
      success: function(response) {
        // If new datePopup settings were added, add our own onClose handler.
        // We need to do this before calling the returned callbacks.
        if (response.scripts.setting.datePopup) {
          for(var id in response.scripts.setting.datePopup) {
            response.scripts.setting.datePopup[id].settings['onClose'] = Drupal.editablefields.datepickerOnClose;
          }
        }

        // Call all callbacks.
        if (response.__callbacks) {
          $.each(response.__callbacks, function(i, callback) {
            eval(callback)(element, response);
          });
        }
        $(element).html(response.content);

        var isAjaxEditable = $(element).hasClass('ajax-editable');

        Drupal.attachBehaviors(element);
        var uniqNum = Drupal.editablefields.checkbox_fix_index++;
        $(element).find(':input').not(':hidden').each(function() {
          var $this = $(this);

          // Create a unique id field for checkboxes 
          if ($this.attr("type") == 'checkbox' || $this.attr("type") == 'radio') {
            $this.attr("id", $this.attr("id") + '-' + uniqNum);
            // change the "for" attribute for the label so that we can still click on it
            if($this.parent('label').length) {
              $this.parent('label').attr("for", $this.parent('label').attr("for") + '-' + uniqNum);
            }
          }

          // attach onChange event only for ajax-editable fields
          if(isAjaxEditable) {
            $this.change(function() {
              Drupal.editablefields.onchange(this);
            });
          }

          // datepicker fields are handled by the Drupal.editablefields.datepickerOnClose handler
          //if (!$('[id*="datepicker"]', $this)) {
            // add blur handler
            $this.blur(function(event) {
              if($this.attr('type') == 'checkbox' || $this.attr('type') == 'radio') {
                if(!$(event.originalEvent.explicitOriginalTarget).parents('.editablefields').length) {
                  Drupal.editablefields.onblur($this);
                }
              }
              else {
                window.setTimeout(function () {
                  Drupal.editablefields.onblur($this);
                }, 10);
              }
            });
          //}

          // Autofocus loaded elements. We need a small timeout here.
          if(!isAjaxEditable) {
            window.setTimeout(function(){
              $this.focus();
            }, 20);
          }
        });

        $(element).removeClass('editablefields_throbber');
      },
      error: function(response) {
        $(".messages.error").remove();
        $(element).after('<div class="messages error">' + Drupal.t("An error occurred at ") + url + '</div>');
        $(".messages.error").hide(0).show(1000);
        $(element).removeClass('editablefields_throbber');
      },
      dataType: 'json'
    });
  }
};

Drupal.editablefields.onchange = function(element) {
  if (!$(element).hasClass('editablefields')) {
    element = $(element).parents('div.editablefields');
  }

  // Provide some feedback to the user while the form is being processed.
  $(element).addClass('editablefields_throbber');

  if ($(element).hasClass('clicktoedit')) {
    // Send the field form for a 'clicktoedit' field.
    $.ajax({
      type: "POST",
      url: Drupal.settings.editablefields.url_submit, 
      data: $(element).find('form').serialize() + "&nid=" + $(element).attr("nid") + "&field=" + $(element).attr("field")+ "&delta=" + $(element).attr("delta"),
      element: $(element),
      success: function(msg) {
        $(element).removeClass('editablefields_throbber');
        $(".messages.error").hide(1000, function() {
          $(this).remove();
        });
        Drupal.editablefields.view(element);
      },
      error: function(msg) {
        $(".messages.error").remove();
        $(element).after('<div class="messages error">' + msg.responseText + '</div>');
        $(".messages.error").hide(0).show(1000);
        $(element).removeClass('editablefields_throbber');
        Drupal.editablefields.load(element);
      }
    });
  }
  else {
    // Send the field form for a 'editable' field.
    $.ajax({
      type: "POST",
      url: Drupal.settings.editablefields.url_submit, 
      data: $(element).find('form').serialize() + "&nid=" + $(element).attr("nid") + "&field=" + $(element).attr("field")+ "&delta=" + $(element).attr("delta"),
      element: $(element),
      success: function(msg) {
        $(element).removeClass('editablefields_throbber');
        // Re-enable the widget.
        $(".messages.error").hide(1000, function() {
          $(this).remove();
        });
        $(element).find(':input').each(function() {
          $(this).attr("disabled", false);
        });
      },
      error: function(msg) {
        $(".messages.error").remove();
        $(element).after('<div class="messages error">' + msg.responseText + '</div>');
        $(".messages.error").hide(0).show(1000);
        $(element).removeClass('editablefields_throbber');
        Drupal.editablefields.load(element);
      }
    });
  }

  // Ensure same changes are not submitted more than once.
  $(element).find(':input').each(function() {
    $(this).attr("disabled", true);
  });

  // Do not actually submit.
  return false;
};

Drupal.editablefields.onblur = function(element, forceClose) {
  // datepicker fields should collapse only when the user closes the matrix display
  if (!forceClose && $(element).hasClass('hasDatepicker')) {
    // this means that this handler has been called by the Drupal.editablefields.datepickerOnClose handler
    return;
  }

  if (!$(element).hasClass('editablefields')) {
    element = $(element).parents('div.editablefields');
  }

  if ($(element).hasClass('clicktoedit')) {
    $(".messages.error").hide(1000, function() {
      $(this).remove();
    });
    $(element).parents('div.field').find('.highlighted').removeClass('highlighted');
    Drupal.editablefields.view(element);
  }
};

/**
 * OnClose handler for datepicker fields.
 * This makes sure that clicktoedit datepicker fields automatically
 * blur when the datepicker gets closed. Otherwise we will have multiple
 * datepickers with the same ID on the page.
 */
Drupal.editablefields.datepickerOnClose = function(dateText, inst) {
  Drupal.editablefields.onblur($(this), true);
};

// CTools AJAX command that uses the active_elements array to
// replace an element on the client side
Drupal.CTools.AJAX.commands.replace_active_element = function(data) {
  var data_object = $(data.data),
    active_element = Drupal.editablefields.active_elements[data.active_elem_index];

  // remove the active element from the array without
  // updating the length property of the array
  delete Drupal.editablefields.active_elements[data.active_elem_index];
  // replace
  $(active_element).replaceWith(data_object);
  // attach behaviors
  Drupal.attachBehaviors(data_object);
};
