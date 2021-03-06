<?php
namespace MRBS;

/*****************************************************************************\
*                                                                             *
*   File name       session_php.inc                                           *
*                                                                             *
*   Description     Use PHP built-in sessions handling                        *
*                                                                             *
*   Notes           To use this authentication scheme, set in                 *
*                   config.inc.php:                                           *
*                       $auth["session"]  = "php";                            *
*                                                                             *
\*****************************************************************************/

require_once MRBS_ROOT . '/functions_logon.inc';

global $PHP_SELF, $HTTP_REFERER;
global $auth;

// Get non-standard form variables
$Action = get_form_var('Action', 'string');
$NewUserName = get_form_var('NewUserName', 'string');
$NewUserPassword = get_form_var('NewUserPassword', 'string');
$target_url = get_form_var('target_url', 'string');
$returl = get_form_var('returl', 'string');

// We need to preserve the original calling page, so that it's there when we eventually get to the
// target_url (especially if that's edit_entry.php).  If this is the first time through then $HTTP_REFERER
// holds the original caller.    If this is the second time through we will have stored it in $returl.
if (!isset($returl))
{
  $returl = isset($HTTP_REFERER) ? $HTTP_REFERER : '';
}
  
if (isset($cookie_path_override))
{
  $cookie_path = $cookie_path_override;
}
else
{
  $cookie_path = $PHP_SELF;
  // Strip off everything after the last '/' in $PHP_SELF
  $cookie_path = preg_replace('/[^\/]*$/', '', $cookie_path);
}

if (!isset($auth["session_php"]["session_expire_time"]))
{
  // Default to the behaviour of previous versions of MRBS, use only
  // session cookies - no persistent cookie.
  $auth["session_php"]["session_expire_time"] = 0;
}

session_name("MRBS_SESSID");  // call before session_set_cookie_params() - see PHP manual
session_set_cookie_params($auth["session_php"]["session_expire_time"],
                          $cookie_path);
                          
// Checking the result of session_start() is not very reliable.   Before PHP 5.3.0
// it always returned TRUE.   Even after 5.3.0 it seems to return TRUE even if the
// session_save_path is not writable.  A warning is generated, but session_start()
// still returns TRUE.
if (FALSE === session_start())
{
  trigger_error("MRBS: could not start session", E_USER_WARNING);
  fatal_error("MRBS: server configuration error.");
}

// Check to see if we've been inactive for longer than allowed and if so logout
// the user
if (!empty($auth["session_php"]["inactivity_expire_time"]))
{
  if (isset($_SESSION['LastActivity']) &&
      ((time() - $_SESSION['LastActivity']) > $auth["session_php"]["inactivity_expire_time"]))
  {
    unset($_SESSION["UserName"]);
  }
  // Ajax requests don't count as activity, unless it's the special Ajax request used
  // to record client side activity
  $ajax = get_form_var('ajax', 'int');
  $activity = get_form_var('activity', 'int');
  if ($activity || !$ajax || !isset($_SESSION['LastActivity']))
  {
    $_SESSION['LastActivity'] = time();
  }
}

/*
  Target of the form with sets the URL argument "Action=SetName".
  Will eventually return to URL argument "target_url=whatever".
*/
if (isset($Action) && ($Action == "SetName"))
{
  /* First make sure the password is valid */
  if ($NewUserName == "")
  {

    // Unset the session variables
    session_unset();
    session_destroy();
  }
  else
  {
    if (($valid_username = authValidateUser($NewUserName, $NewUserPassword)) === false)
    {
      print_header(0, 0, 0, 0, "");
      echo "<p>".get_vocab('unknown_user')."</p>\n";
      printLoginForm(this_page(), $target_url, $returl);
      exit();
    }
    $_SESSION["UserName"] = $valid_username;
  }
  // preserve the original $HTTP_REFERER by sending it as a GET parameter
  if (!empty($returl))
  {
    // check to see whether there's a query string already
    if (strpos($target_url, '?') === false)
    {
      $target_url .= "?returl=" . urlencode($returl);
    }
    else
    {
      $target_url .= "&returl=" . urlencode($returl);
    }
  }
  // Problems have been reported on Windows IIS with session data not being
  // written out without a call to session_write_close()
  session_write_close();
  header ("Location: $target_url"); /* Redirect browser to initial page */
  /* Note HTTP 1.1 mandates an absolute URL. Most modern browsers support relative URLs,
    which allows to work around problems with DNS inconsistencies in the server name.
    Anyway, if the browser cannot redirect automatically, the manual link below will work. */
  print_header(0, 0, 0, 0, "");
  echo "<br>\n";
  echo "<p>Please click <a href=\"".htmlspecialchars($target_url)."\">here</a> if you're not redirected automatically to the page you requested.</p>\n";

  // Print footer and exit
  print_footer(TRUE);
}


/*
  Target of the form with sets the URL argument "Action=QueryName".
  Will eventually return to URL argument "target_url=whatever".
*/
if (isset($Action) && ($Action == "QueryName"))
{
  print_header(0, 0, 0, 0, "");
  printLoginForm(this_page(), $target_url, $returl);
  exit();
}


/* authGet()
 * 
 * Request the user name/password
 * 
 * Returns: Nothing
 */
function authGet()
{
  global $QUERY_STRING;
  global $returl;

  print_header(0, 0, 0, 0, "");

  echo "<p>".get_vocab("norights")."</p>\n";

  $target_url = this_page();
  if (isset($QUERY_STRING))
  {
    $target_url = $target_url . "?" . $QUERY_STRING;
  }
  printLoginForm(this_page(), $target_url, $returl);

  exit();
}


function getUserName()
{
  if (isset($_SESSION["UserName"]) && ($_SESSION["UserName"] !== ''))
  {
    return $_SESSION["UserName"];
  }

  return NULL;
}


// Print the logon entry on the top banner.
function PrintLogonBox()
{
  global $QUERY_STRING, $user_list_link, $day, $month, $year;

  $target_url = this_page();
  if (isset($url_base) && ($url_base !== ''))
  {
    $target_url = $url_base . '/' . $target_url;
  }
  if (isset($QUERY_STRING))
  {
    $target_url = $target_url . "?" . $QUERY_STRING;
  }
  $user=getUserName();
  if (isset($user))
  {
    // words 'you are xxxx' becomes a link to the
    // report page with only entries created by xxx. Past entries are not
    // displayed but this can be changed
    $search_string = "report.php?from_day=$day&amp;from_month=$month&amp;".
      "from_year=$year&amp;to_day=1&amp;to_month=12&amp;to_year=2030&amp;areamatch=&amp;".
      "roommatch=&amp;namematch=&amp;descrmatch=&amp;summarize=1&amp;sortby=r&amp;display=d&amp;".
      "sumby=d&amp;creatormatch=".urlencode($user); ?>

                <a href="<?php echo "$search_string\" title=\""
         . get_vocab('show_my_entries') . "\">" . get_vocab('you_are')." "
         . htmlspecialchars($user) ?></a>
                <form method="post" action="admin.php">
                  <div>
                    <input type="hidden" name="target_url" value="<?php echo htmlspecialchars($target_url) ?>">
                    <input type="hidden" name="Action" value="SetName">
                    <input type="hidden" name="NewUserName" value="">
                    <input type="hidden" name="NewUserPassword" value="">
                    <input type="submit" value=" <?php echo get_vocab('logoff') ?> ">
                  </div>
                </form>
<?php
  }
  else
  {
?>
                <a href=""><?php echo get_vocab('unknown_user'); ?></a>
                <form method="post" action="admin.php">
                  <div>
                    <input type="hidden" name="target_url" value="<?php echo htmlspecialchars($target_url) ?>">
                    <input type="hidden" name="Action" value="QueryName">
                    <input type="submit" value=" <?php echo get_vocab('login') ?> ">
                  </div>
                </form>
<?php
  } 
  if (isset($user_list_link))
  {
    print "<a id=\"user_list_link\" href=\"$user_list_link\">" . get_vocab('user_list') . "</a>\n";
  }
}

