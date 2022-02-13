#include "paths.h"
#include <FS.h>
#include <format.h>
#include <shared.h>

namespace paths
{
static const char *legacy_config_dir = "~/.timewarrior";
static const bool uses_legacy_config = Directory (legacy_config_dir).exists ();
const char *timewarriordb = getenv ("TIMEWARRIORDB");

#ifdef __unix__
std::string getPath (const char *xdg_path)
{
  if (timewarriordb != nullptr)
  {
    return timewarriordb;
  }
  else if (uses_legacy_config)
  {
    return legacy_config_dir;
  }
  else
  {
    return std::string (xdg_path) + "/timewarrior";
  }
}

const char *getenv_default (const char *env, const char *default_value)
{
  const char *value = getenv (env);
  if (value == nullptr)
  {
    return default_value;
  }

  return value;
}

static std::string conf_dir = getPath (getenv_default ("XDG_CONFIG_HOME", "~/.config"));
static std::string data_dir = getPath (getenv_default ("XDG_DATA_HOME", "~/.local/share"));
std::string configDir () { return conf_dir; }
std::string dataDir () { return data_dir; }

#else
std::string getPath ()
{
  if (timewarriordb != nullptr)
  {
    return timewarriordb;
  }
  else
  {
    return legacy_config_dir;
  }
}

static std::string path = getPath ();

std::string configDir ()
{
    return path;
}
std::string dataDir ()
{
    return path;
}

#endif

std::string configFile () { return configDir () + "/timewarrior.cfg"; }
std::string dbDir () { return dataDir () + "/data"; }
std::string extensionsDir () { return configDir () + "/extensions"; }

bool createDirs (const CLI &cli)
{
  Directory configLocation = Directory (configDir ());
  bool configDirExists = configLocation.exists ();

  if (configDirExists &&
      (!configLocation.readable () ||
       !configLocation.writable () ||
       !configLocation.executable ()))
  {
    throw format ("Config is not readable at '{1}'", configLocation._data);
  }

  Directory dataLocation = Directory (dataDir ());
  bool dbDirExists = dataLocation.exists ();
  if (dbDirExists &&
          (!dataLocation.readable () ||
           !dataLocation.writable () ||
           !dataLocation.executable ()))
  {
    throw format ("Database is not readable at '{1}'", dataLocation._data);
  }

  std::string question = "";
  if (!configDirExists)
  {
    question += "Create new config in " + configLocation._data + "?";
  }
  if (!dbDirExists && configLocation._data != dataLocation._data)
  {
    if (question != "") {
        question += "\n";
    }
    question += "Create new database in " + dataLocation._data + "?";
  }

  if (!configDirExists || !dbDirExists)
  {
    if (cli.getHint ("yes", false) || confirm (question))
    {
      if (!configDirExists)
      {
        configLocation.create (0700);
      }
      if (!dbDirExists)
      {
        dataLocation.create (0700);
      }
    }
  }

  // Create extensions subdirectory if necessary.
  Directory extensionsLocation (extensionsDir ());

  if (!extensionsLocation.exists ())
  {
    extensionsLocation.create (0700);
  }

  // Create data subdirectory if necessary.
  Directory dbLocation (dbDir ());

  if (!dbLocation.exists ())
  {
    dbLocation.create (0700);
  }

  // Load the configuration data.
  Path configFileLocation (configFile ());

  if (!configFileLocation.exists ())
  {
    File (configFileLocation).create (0600);
  }

  return !dbDirExists;
}

} // namespace paths
