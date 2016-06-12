#include "util/mysql/mysql.h"

DEF_string(mysql_ip, "192.168.1.28", "ip for mysql");
DEF_uint32(mysql_port, 3306, "port for mysql");
DEF_string(mysql_user, "root", "user name for mysql");
DEF_string(mysql_pwd, "12345", "password for mysql");
DEF_string(mysql_db, "testecpp", "dbname for mysql");

static void printMysqlData(const util::Mysql::MysqlData &data) {
  if (data.empty()) {
    LOG<<"nothing to print";
    return;
  }

  for (auto it = data.begin(); it != data.end(); ++it) {
    const auto &map = *it;
    for (auto it_data = map.begin(); it_data != map.end(); ++it_data) {
      LOG<< "key: " << it_data->first << ", value: " << it_data->second;
    }
  }
}

static void simpleTest(std::shared_ptr<util::MysqlClient> db) {
  const std::string table_name("testtable");
  const std::string sql_create(
      "create table " + table_name + "(id int, name varchar(255))");
  const std::string sql_select("select * from " + table_name);
  const std::string sql_insert("(id,name) values(3,'hello')");
  const std::string sql_query("select * from " + table_name);
  const std::string sql_update("id=4, name='helloagin'");
  const std::string sql_del("where id=3");
  const std::string sql_del_table("drop table " + table_name);

  CHECK((*db)->execute(sql_create)) << " create " << table_name
                                     << " fail using: " << sql_create;
  CHECK((*db)->insert_record(table_name, sql_insert)) << " insert "
                                                       << table_name
                                                       << " using: "
                                                       << sql_insert;

  util::Mysql::MysqlData data = (*db)->query_record(sql_query);
  printMysqlData(data);

  CHECK((*db)->update_record(table_name, sql_update)) << " update "
                                                       << table_name
                                                       << " using: "
                                                       << sql_update;
  data = (*db)->query_record(sql_query);
  printMysqlData(data);

  CHECK((*db)->del_record(table_name, sql_del)) << " update " << table_name
                                                 << " using: " << sql_update;
  data = (*db)->query_record(sql_query);
  printMysqlData(data);

  CHECK((*db)->execute(sql_del_table)) << " del " << table_name
                                        << " fail using: " << sql_del_table;
}

int main(int argc, char** argv) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  std::shared_ptr<util::MysqlClient> mysql(
      util::CreateMysqlClient(FLG_mysql_ip, FLG_mysql_port, FLG_mysql_user,
                              FLG_mysql_pwd, FLG_mysql_db));
  CHECK_NOTNULL(mysql);

  CHECK((*mysql)->connect()) << " connet to mysql error: " << FLG_mysql_ip
                              << ", port: " << FLG_mysql_port << ", user: "
                              << FLG_mysql_user << ", password: "
                              << FLG_mysql_pwd << ", db: " << FLG_mysql_db;

  simpleTest(mysql);

  while (1) {
    sys::sleep(1);
  }

  return 0;
}
