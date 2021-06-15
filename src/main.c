#include <sqlite3/sqlite3.h>
#include <iron/full.h>
#include <sys/stat.h>
#include <dirent.h>

const char * print_sqlite_code(int code){
  switch(code){
  case SQLITE_BUSY: return "BUSY";
  case SQLITE_DONE: return "DONE";
  case SQLITE_ROW: return "ROW";
  case SQLITE_ERROR: return "ERROR";
  case SQLITE_MISUSE: return "MISUSE";
  default: ERROR("NO SUCH CODE");
    return "<ERROR>";
  }
}

typedef enum {
	      SQL_END = 0,
	      SQL_INT,
	      SQL_DOUBLE,
	      SQL_STRING,
}sql_value_type;

void sql_exec_va(sqlite3_stmt ** cache, bool log, sqlite3 * db, const char * sql, va_list args){
  sqlite3_stmt * stmt;
  if(cache){
    if(*cache == NULL){
      sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
      *cache = stmt;
    }else{
      stmt = *cache;
      sqlite3_clear_bindings(stmt);
    }
  }
  else{
    sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
  }
  int parmCount = sqlite3_bind_parameter_count(stmt);
  int i = 0;
  while(true){
    i += 1;
    sql_value_type t = va_arg(args, int);
    switch(t){
    case SQL_END: goto end;
    case SQL_INT:
      {	
	i64 v = va_arg(args, i64);
	sqlite3_bind_int64(stmt, i, v);
	break;
      }
    case SQL_DOUBLE:
      {
	f64 v = va_arg(args, f64);
	sqlite3_bind_double(stmt, i, v);
	break;
      }
    case SQL_STRING:
      {
	char * v = va_arg(args, char *);
	sqlite3_bind_text(stmt, i, v, strlen(v), SQLITE_TRANSIENT);
	break;
      }
    }
  }
 end:;

  bool first_row = true;
  while(true){
    int ok = sqlite3_step(stmt);
    if(ok == SQLITE_DONE)
      break;
    if(log && ok == SQLITE_ROW){
      int count = sqlite3_data_count(stmt);
      int column_count = sqlite3_column_count(stmt);
      
      if(first_row){
	first_row = false;
	for(int i = 0; i < column_count; i++){
	  if(i > 0)
	    logd(", ");
	  logd("%s", sqlite3_column_name(stmt, i));
	}
	logd("\n");
      }
      for(int i = 0; i < count; i++){
	if(i > 0)
	  logd(", ");
	var t = sqlite3_column_type(stmt, i);
	
	if(t == SQLITE_INTEGER)
	  logd("%lld", sqlite3_column_int64(stmt, i));
	if(t == SQLITE_TEXT)
	  logd("%s", sqlite3_column_text(stmt, i));
	if(t == SQLITE_FLOAT)
	  logd("%f", sqlite3_column_double(stmt, i));
      }
      logd("\n");
    }
    
    if(ok == SQLITE_ERROR){
      const char * err = sqlite3_errmsg(db);
      ERROR("ERROR occured: %s", err);
      break;
    }
    if(ok == SQLITE_MISUSE){
      const char * err = sqlite3_errmsg(db);
      ERROR("Misuse occured: %s", err);
      break;
    }
  }
  if(cache == NULL)
    sqlite3_finalize(stmt);
}

void sql_exec(sqlite3 * db, const char * sql, ...){
  va_list args;
  va_start(args, sql);
  sql_exec_va(NULL, false, db, sql, args);
  va_end(args);
}

void sql_exec3(sqlite3 * db, bool log, const char * sql, ...){
  va_list args;
  va_start(args, sql);
  sql_exec_va(NULL, log, db, sql, args);
  va_end(args);
}

void sql_query(sqlite3 * db, const char * sql){
  sql_exec(db, sql);
}

const char * mode_to_string(struct stat sb){
  
  switch (sb.st_mode & S_IFMT) {
  case S_IFBLK:  return("block device");            break;
  case S_IFCHR:  return("character device");        break;
  case S_IFDIR:  return("directory");               break;
  case S_IFIFO:  return("FIFO/pipe");               break;
  case S_IFLNK:  return("symlink");                 break;
  case S_IFREG:  return("regular file");            break;
  case S_IFSOCK: return("socket");                  break;
  default:       return("unknown?");                break;
  }
}

bool stat_is_dir(struct stat sb){
  return  S_IFDIR == (sb.st_mode & S_IFMT);
}
bool stat_is_reg(struct stat sb){
  return  S_IFREG == (sb.st_mode & S_IFMT);
}
int stat_perm(struct stat sb){
  return sb.st_mode & 0777;
}

const char * dirent_to_type(int dirent_type){
  switch(dirent_type){
  case DT_BLK: return "block device";
  case DT_CHR: return "character device";
  case DT_DIR: return "directory";
  case DT_FIFO: return "named pipe";
  case DT_LNK: return "symbolic link";
  case DT_REG: return "regular file";
  case DT_SOCK: return "domain socket";
  case DT_UNKNOWN: return "unknown";
  default: return "no-type";
  }
 }


void print_files_table(sqlite3 * db){
  const char * sql = "SELECT * from files";
  sql_exec3(db, true, sql);
}

void update_run(sqlite3 *db, const char * loc_path){
  struct stat sb;
  stat(loc_path, &sb);
  if(stat_is_dir(sb)){
    //logd("DIR: %s\n", loc_path);
    DIR * d = opendir(loc_path);
    if(d == NULL)
      return;
    
    sql_exec(db, "insert or ignore into local.temp_files (name, type) VALUES (?, ?)", SQL_STRING, loc_path, SQL_INT, 1, SQL_END);
    
    
    struct dirent * ent;
    while((ent = readdir(d)) != NULL){
      if(strcmp(".", ent->d_name) == 0
	 || strcmp("..", ent->d_name) == 0)
	continue; // skip these
      var sub = fmtstr("%s/%s", loc_path, ent->d_name);
      sql_exec(db, "INSERT INTO local.to_process (name) VALUES (?)", SQL_STRING, sub, SQL_END);
      
      dealloc(sub);
    }
    closedir(d);
    
  }else if(stat_is_reg(sb)){
    //logd("FILE: %s\n", loc_path);
    //sql_exec_str(db, "insert into files (name) VALUES (?)", loc_path);
    sql_exec(db, "insert or ignore into local.temp_files (name, type, size, modified, created, permissions) VALUES (?, ?, ?, ?, ?, ?)", SQL_STRING, loc_path, SQL_INT, 2, SQL_INT, sb.st_size, SQL_INT, sb.st_mtime, SQL_INT, sb.st_ctime, SQL_INT, stat_perm(sb), SQL_END);    
  }
  sql_exec(db, "UPDATE local.to_process SET finished = 1 WHERE name = ?", SQL_STRING, loc_path, SQL_END);
}

bool process_files(sqlite3 * db){
  sql_exec(db, "BEGIN TRANSACTION");
  const char * sql = "SELECT name FROM local.to_process WHERE finished IS NULL LIMIT 10";
  sqlite3_stmt * stmt;
  sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
  char * files[10] = {0};
  int i = 0;
  int status;
  while((status = sqlite3_step(stmt)) == SQLITE_ROW){
    ASSERT(i < 10);
    const char * file = sqlite3_column_text(stmt, 0);
    files[i] = fmtstr("%s", file);
    i++;
  }
  sqlite3_finalize(stmt);
  if(status == SQLITE_ERROR || status == SQLITE_MISUSE){
    const char * err = sqlite3_errmsg(db);
    ERROR("ERROR occured: %s", err);
  }
  if(i == 0){
    sql_exec(db, "COMMIT");
    return false;
  }
  for(int j = 0; j < i; j++){
    if(files[j] == NULL)
      break;
    logd("processing: %s\n", files[j]);
    update_run(db, files[j]);
    dealloc(files[j]);
    files[j] = NULL;
  }
  
  sql_exec(db, "DELETE FROM local.to_process WHERE finished = 1");
  sql_exec(db, "COMMIT");
  return true;
}


int main(int argc, char ** argv){
  sqlite3_initialize();

  sqlite3 *db = 0;
  sqlite3_open("main.sql", &db);
  //sql_exec(db, "ATTACH DATABASE 'main.sql' as main");
  sql_exec(db, "ATTACH DATABASE 'local.sql' as local");
  sql_query(db, "pragma user_version = 150");
  sql_query(db, "pragma user_version");
  sql_query(db, "create table IF NOT EXISTS files (name TEXT PRIMARY KEY, type INTEGER, size BIGINTEGER, modified BIGINTEGER, created BIGINTEGER, permissions INTEGER);");

  // temp_files should contain the current state of the calculation
  // in addition to the fact if a thing has been processed to completion
  // the minimal amount of processing differs for different types of objects
  // for extremely large collections of files, the update operation may take a significant amount of time, therefore, it should be done in some chunks locally. It may be best if this part of the operation is contained in a separate macine-local file.
  // for this purpose, the ATTACH statement might be the right way to make joins from a local DB to the backed DB.
  // note: consider using the parent folder AND file name as combined PRIMARY KEy.
  // that will reduce the amount of overhead in storing the files and make us
  // immune to the problem of directory separators that needs to be handled.
  // eg:
  //column1 INTEGER NOT NULL,
  //column2 INTEGER NOT NULL,
  //PRIMARY KEY ( column1, column2)
  // one problem is that it might make it hard to move from local to main table.// however having to handle ALL possible file names is going to be  PAIN
  // so it is probably better to just find a solution to that problem.
  sql_query(db, "create table IF NOT EXISTS local.temp_files (name TEXT PRIMARY KEY, type INTEGER, size BIGINTEGER, modified INTEGER, created INTEGER, permissions INTEGER, touch INTEGER);");
  sql_query(db, "create table IF NOT EXISTS local.to_process (name TEXT PRIMARY KEY, finished INTEGER);");
  sql_exec(db, "BEGIN TRANSACTION");
  //update_run(db, "/home/romadsen/syncthing");
  update_run(db, "./iron");
  sql_exec(db, "COMMIT");
  
  logd("EH:\n");
  while(process_files(db)){
    sql_exec3(db, true, "SELECT * FROM local.to_process LIMIT 10");
  
  }
  //sql_exec3(db, true, "SELECT name FROM local.to_process");
  process_files(db);
  sql_exec3(db, true, "SELECT * FROM local.to_process");
  sqlite3_close(db);
  //sql_query(db, "create temp table zero (touch INTEGER)");
  //sql_query(db, "INSERT INTO zero (touch) VALUES (0)");
  
  //update_run(db, "./iron");
  //sql_query(db, "INSERT INTO temp_files SELECT * FROM files JOIN zero");
  //sql_exec3(db, true, "SELECT count(name), sum(size)/1000000 from temp_files");
  //sql_exec3(db, true, "SELECT * FROM temp_files");
  //print_files_table(db);
  return 0;
  
}
