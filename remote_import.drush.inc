<?php
// $Id$

/**
 * @file
 *   An remote_import of the provision service API.
 *
 *  Declares a new service type and a basic implementation of it.
 *  It matches the same service definition in the hosting front end.
 */

include_once(dirname(__FILE__) . '/../provision.service.inc');

/**
 * Expose the service type this extension defines to provision.
 *
 * Implementations are then conditionally loaded when the option 
 * "--$service_service_type=$type" is passed to the provision-save
 * command of a server.
 *
 * Implementations are automatically loaded from :
 * provision/$service/$type/$type_service.inc.
 *
 * @return
 *   An array with the service type the key, and the default implementation the value.
 */
function remote_import_provision_services() {
  return array('remote_import' => NULL);
}

/**
 * The service type base class.
 *
 * All implementations of the service type will inherit this class.
 * This class should define the 'public API' to be used by the rest
 * of the system, which should not expose implementation details.
 */
class Provision_Service_remote_import extends Provision_Service {
  public $service = 'remote_import';

  /**
   * Initialize the service along with the server object.
   */
  function init() {
    // REMEMBER TO CALL THE PARENT!
    parent::init();
  }

  /**
   * Called on provision-verify.
   *
   * We change what we will do based on what the 
   * type of object the command is being run against.
   */
  function verify_server_cmd() {
    if ($this->context->type == 'server') {
      // We need to make sure that this is a REMOTE server!
      if (provision_is_local_host($this->server->remote_host)) {
        return drush_set_error('REMOTE_SERVER_IS_LOCAL', dt('The specified server is not a remote server.'));
      }
    }
  }
  
  function list_sites() {
    return array();
  }
  
  function fetch_site() {
    return FALSE;
  }
  
  function fetch_settings($old_url) {
    return array();
  }
  
  function deploy($backup_file, $old_uri, $new_uri, $platform, $db_server) {
    // Need to create a context for this one.
    $options = array();
    $options['context_type'] = 'site';
    $options['uri'] = $new_uri;
    $hash_name = drush_get_option('#name') ? '#name' : 'name';
    $options[$hash_name] = $new_uri;
    $options['platform'] = '@' . $platform;
    $options['root'] = d($options['platform'])->root;
    $options['aliases'] = array();
    $options['redirection'] = 0;
    $options['db_server'] = '@' . $db_server;
    $options += $this->fetch_settings($old_uri);
    
  
    drush_backend_invoke_args('provision-save', array('@' . $new_uri), $options);
  
    provision_backend_invoke($new_uri, 'provision-deploy', array($backup_file), array('old_uri' => $old_uri));
  
    if (!drush_get_error()) {
      provision_backend_invoke($new_uri, 'provision-verify');
      return '@' . $new_uri;
    }
  }
}

/**
 * Implements hook_drush_command().
 */
function remote_import_drush_command() {
  $items = array();
   
  $items['provision-remote_import_list_sites'] = array(
    'description' => "List all sites on the remote server.",
    'arguments' => array(
    ),
    'options' => array(
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
  
  $items['provision-remote_import'] = array(
    'description' => "Imports a site from the remote server.",
    'arguments' => array(
    ),
    'options' => array(
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
   
  return $items;
}

function drush_remote_import_provision_remote_import_list_sites() {
  $sites = d()->service('remote_import')->list_sites();
  if (is_array($sites)) {
    drush_set_option('remote_sites_list', $sites);
  }
  else {
    return drush_set_error();
  }
}

function drush_remote_import_provision_remote_import() {
  $old_url = drush_get_option('old_url');
  
  // Go do a backup of that site.
  $backup_file = d()->service('remote_import')->fetch_site($old_url);
  
  // Now we want to deploy that backup with the given options.
  $new_alias = d()->service('remote_import')->deploy($backup_file, drush_get_option('old_url'), drush_get_option('new_url'), drush_get_option('platform'), drush_get_option('db_server'));
  
  // And finally, we should import this back into Hostmaster.
  provision_backend_invoke('@hostmaster', 'hosting-import', array($new_alias), array('remote_import_import' => TRUE));
}
