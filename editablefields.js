// $Id$

Drupal.behaviors.editablefields = function(context) {
  $('div.editablefields', context).not('.clicktoedit').not('.editablefields-processed').each(function() {
    $(this).addClass('editablefields-processed');
    Drupal.editablefields.load(this);
  });
  $('div.editablefields', context).filter('.clicktoedit').not('.editablefields-processed').each(function() {
    $(this).prepend(Drupal.settings.editablefields.clicktoedit_message);
    $(this).click(Drupal.editablefields.init);
  });
  $('div.field-label:not(.label-processed)', context).addClass('label-processed').each(function() {
    $(this).click(function() {
      $(this).parent().find('.editablefields').each(function() {
        Drupal.editablefields.load(this);
      });
      return false;
    });
  });
  $('div.field-label:has(.label-processed)', context).removeClass('label-processed').each(function() {
    $(this).mousedown(function() {
      $(this).parent().find('.editablefields').each(function() {
        Drupal.editablefields.view(this);
      });
      return false;
    });
  });
  $('div.editablefields', context).not('.editablefields-processed').submit(function(){
    return false;
  });
}


Drupal.editablefields = {};

Drupal.editablefields.init = function() {
  $(this).unbind("click",Drupal.editablefields.init);
  $(this).addClass('editablefields-processed');
  $(this).children().hide();
  Drupal.editablefields.load(this);
}

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
        //alert(response.content);
        $(element).html(response.content);
        Drupal.attachBehaviors(element);
        //$(element).prepend(Drupal.settings.editablefields.clicktoedit_message);
        $(element).bind("click",Drupal.editablefields.init);
        $(element).removeClass('editablefields_throbber');
        $(element).removeClass('editablefields-processed');
      },
      error: function(response) {
        alert(Drupal.t("An error occurred at ") + url);
        $(element).removeClass('editablefields_throbber');
        $(element).removeClass('editablefields-processed');
      },
      dataType: 'json'
    });
  }
};

Drupal.editablefields.load = function(element) {

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
        // Call all callbacks.
        if (response.__callbacks) {
          $.each(response.__callbacks, function(i, callback) {
            eval(callback)(element, response);
          });
        }
        $(element).html(response.content);
        Drupal.attachBehaviors(element);
        $(element).find(':input').change(function() {
          Drupal.editablefields.onchange(this);
        });
        $(element).find(':input').blur(function() {
          Drupal.editablefields.onblur(this);
        });
        if ($(element).find(':input').not(':hidden').hasClass('form-text')) {
          $(element).find(':input').not(':hidden').get(0).focus();
        }
        if ($(element).find(':input').not(':hidden').hasClass('form-radio')) {
          $(element).find(':checked').not(':hidden').get(0).focus();
        }
        if ($(element).find(':input').not(':hidden').hasClass('form-checkbox')) {
          $(element).find(':checked').not(':hidden').get(0).focus();
        }
        if ($(element).find('select').not(':hidden').hasClass('form-select')) {
          $(element).find(':selected').not(':hidden').select();
          $(element).find('select').not(':hidden').focus();
        }
        if ($(element).find(':input').not(':hidden').hasClass('form-submit')) {
          //$(element).find(':selected').not(':hidden').select();
          $(element).find('.form-submit').not(':hidden').focus();
        }
//        if ($(element).find(':input').not(':hidden').hasClass('form-textarea')) {
//          $(element).find('.form-textarea').not(':hidden').focus();
//        }
        $(element).removeClass('editablefields_throbber');
      },
      error: function(response) {
        alert(Drupal.t("An error occurred at ") + url);
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
        Drupal.editablefields.view(element);
      },
      error: function(msg) {
        alert(Drupal.t("Error, unable to make update:") +"\n"+ msg.responseText);
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
        Drupal.editablefields.load(element);
      },
      error: function(msg) {
        alert(Drupal.t("Error, unable to make update:") +"\n"+ msg.responseText);
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

Drupal.editablefields.onblur = function(element) {
  if (!$(element).hasClass('editablefields')) {
    element = $(element).parents('div.editablefields');
  }

  if ($(element).hasClass('clicktoedit')) {
    Drupal.editablefields.view(element);
  }

  return false;
};
