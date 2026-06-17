# MiniDB 简易数据库项目说明

## 编译步骤

### 1.进入编译目录
 cd mini-db/build

### 2.清理数据
rm - rf *

### 3.CMake构建及编译
cmake ..
make -j4

### 4.单元测试

1.测试Vector、String自研容器
./test-container

2.测试SQL词法、语法解析
./test-parser

3.测试数据库、数据表、行、列基础读写
./test-storage

### 5.完整交互式测试
启动服务端
./minidb-server

另开终端，到build目录下启动客户端
./minidb-client

SQL测试

-- 1. 创建数据库
create database test_db;
-- 2. 切换数据库
use test_db;
-- 3. 创建表，id为主键自动创建B+树索引
create table student(id int primary,name string,age int);

-- 插入多条数据
insert student values(1001,"peter",20);
insert student values(1002,"john",21);
insert student values(1003,"lily",19);

-- 全表查询
select * from student;

-- 索引条件查询
select name from student where id = 1001;
select name,age from student where id < 1003;

-- 更新普通字段（客户端单条执行无主键冲突）
update student set age=22 where id=1001;
select * from student;

-- 删除指定行
delete student where id=1003;
select * from student;

-- 删除数据表
drop table student;
-- 删除数据库
drop database test_db;

-- 退出客户端
exit