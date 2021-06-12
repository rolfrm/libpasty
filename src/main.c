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

void sql_query(sqlite3 * db, const char * sql){
  sqlite3_stmt * stmt = NULL;
  int prep = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
  for(int i = 0; i < 10; i++){
    int ok = sqlite3_step(stmt);
    int count = sqlite3_data_count(stmt);
    for(int i = 0; i < count; i++)
      logd("Value: %i\n", sqlite3_column_int(stmt, i));
    if(ok == SQLITE_DONE){
      break;
    }
  }
  sqlite3_finalize(stmt);
}

int sql_query_int(sqlite3 * db, const char * sql){
  sqlite3_stmt * stmt = NULL;
  int prep = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
  for(int i = 0; i < 10; i++){
    int ok = sqlite3_step(stmt);
    int count = sqlite3_data_count(stmt);
    for(int i = 0; i < count; i++)
      logd("Value: %i\n", sqlite3_column_int(stmt, i));
    if(ok == SQLITE_DONE){
      break;
    }
  }
  sqlite3_finalize(stmt);
  return 0;
}

void sql_exec_str(sqlite3 * db, const char * sql, const char * str){
  sqlite3_stmt * stmt = NULL;
  sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
  sqlite3_bind_text(stmt, 1, str, strlen(str) + 1, SQLITE_TRANSIENT);
  for(int i = 0; i < 10; i++){
    int ok = sqlite3_step(stmt);
    if(ok == SQLITE_DONE)
      break;
  }
  sqlite3_finalize(stmt);
}
typedef enum {
	      SQL_END = 0,
	      SQL_INT,
	      SQL_DOUBLE,
	      SQL_STRING,
}sql_value_type;

void sql_exec(sqlite3 * db, const char * sql, ...){
  sqlite3_stmt * stmt = NULL;
  sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
  int parmCount = sqlite3_bind_parameter_count(stmt);
  va_list args;
  va_start (args, sql);
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
	
	i64 v = va_arg(args, f64);
	sqlite3_bind_double(stmt, i, v);
	break;
      }
    case SQL_STRING:
      {
	char * v = va_arg(args, char *);
	sqlite3_bind_text(stmt, i, v, strlen(v) + 1, SQLITE_TRANSIENT);
	break;
      }
    }
  }
 end:
  va_end(args);

  for(int i = 0; i < 10; i++){
    int ok = sqlite3_step(stmt);
    if(ok == SQLITE_DONE)
      break;
    if(ok == SQLITE_ERROR){
      ERROR("ERROR occured");
      break;
    }
  }
  sqlite3_finalize(stmt);

}
  
void test_io_methods(void){
  sqlite3_io_methods io;
  io.iVersion = 1;
  
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
  logd("FILES\n");
  const char * sql = "SELECT * from files";
  sqlite3_stmt * stmt = NULL;
  int prep = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
  while(true){
    int ok = sqlite3_step(stmt);
    int count = sqlite3_data_count(stmt);
    logd("row: ");
    for(int i = 0; i < count; i++){
      if(i > 0)
	logd(", ");
      var t = sqlite3_column_type(stmt, i);
      if(t == SQLITE_INTEGER)
	logd("%i", sqlite3_column_int(stmt, i));
      if(t == SQLITE_TEXT)
	logd("%s", sqlite3_column_text(stmt, i));
      
    }
    
    logd("\n");
    if(ok == SQLITE_DONE)
      break;
  }
  sqlite3_finalize(stmt);
  
}

void update_run(sqlite3 *db, const char * loc_path){
  struct stat sb;
  stat(loc_path, &sb);
  if(stat_is_dir(sb)){
    //logd("DIR: %s\n", loc_path);
    DIR * d = opendir(loc_path);
    if(d == NULL)
      return;
    sql_exec(db, "insert into files (name, type) VALUES (?, ?)", SQL_STRING, loc_path, SQL_INT, 1, SQL_END);
    
    
    struct dirent * ent;
    while((ent = readdir(d)) != NULL){
      if(strcmp(".", ent->d_name) == 0
	 || strcmp("..", ent->d_name) == 0)
	continue; // skip these
      var sub = fmtstr("%s/%s", loc_path, ent->d_name);
      update_run(db, sub);
      dealloc(sub);
    }
    closedir(d);
  }else if(stat_is_reg(sb)){
    //logd("FILE: %s\n", loc_path);
    //sql_exec_str(db, "insert into files (name) VALUES (?)", loc_path);
    sql_exec(db, "insert into files (name, type, size, modified) VALUES (?, ?, ?, ?)", SQL_STRING, loc_path, SQL_INT, 2, SQL_INT, sb.st_size, SQL_INT, sb.st_mtime, SQL_END);
    
  }
  
}

int main(int argc, char ** argv){
  sqlite3_initialize();

  sqlite3 *db = 0;
  sqlite3_open(":memory:", &db);
  sql_query(db, "pragma user_version = 150");
  sql_query(db, "pragma user_version");
  sql_query(db, "create table files (name TEXT PRIMARY KEY, type INTEGER, size INTEGER, modified INTEGER);");
  update_run(db, "./iron");
  sql_query_int(db, "SELECT count(name) from files");
  print_files_table(db);
  return 0;
  
}
