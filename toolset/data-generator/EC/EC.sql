/*
Navicat MySQL Data Transfer

Source Server         : localhost
Source Server Version : 50717
Source Host           : localhost:3306
Source Database       : ec3

Target Server Type    : MYSQL
Target Server Version : 50717
File Encoding         : 65001

Date: 2017-10-09 14:27:33
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for r_ec_brand
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_brand`;
CREATE TABLE `r_ec_brand` (
  `nBrandID` int(64) NOT NULL AUTO_INCREMENT COMMENT '品牌ID',
  `sBrandName` varchar(64) NOT NULL COMMENT '品牌名称',
  `sCode` varchar(20) NOT NULL DEFAULT '' COMMENT '商品编号',
  `sBriefName` varchar(50) NOT NULL DEFAULT '' COMMENT ' 商品简称',
  `nStatus` char(1) NOT NULL DEFAULT '1' COMMENT '-1:无效 0:初始 1: 有效',
  `sComment` varchar(1024) NOT NULL DEFAULT '' COMMENT '备注',
  `nOrderNum` tinyint(4) NOT NULL DEFAULT '0' COMMENT '排位顺序',
  `dCreateDate` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建日期',
  `dUpdateDate` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`nBrandID`),
  KEY `PRIMARY_nBrandID` (`nBrandID`) USING BTREE COMMENT '使用品牌id作为主键索引'
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8 COMMENT='品牌信息';

-- ----------------------------
-- Table structure for r_ec_cartsku
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_cartsku`;
CREATE TABLE `r_ec_cartsku` (
  `nUserID` bigint(64) NOT NULL DEFAULT '0' COMMENT '用户id',
  `nSKUID` bigint(64) NOT NULL DEFAULT '0' COMMENT 'SKUID',
  `nQuantity` int(64) NOT NULL DEFAULT '0' COMMENT '商品数量-购物车中的商品数量',
  `nShoppingCartID` bigint(64) NOT NULL DEFAULT '0' COMMENT '购物车ID',
  `nCartSKUID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '自增ID',
  `dCreateDate` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '生成时间',
  `dUpdateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`nCartSKUID`),
  KEY `PRIMARY_nUserID` (`nUserID`) USING BTREE COMMENT '使用用户id作为主键索引',
  KEY `PRIMARY_nProductID` (`nSKUID`) USING BTREE COMMENT '使用产品id作为主键索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='购物车SKU';

-- ----------------------------
-- Table structure for r_ec_category
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_category`;
CREATE TABLE `r_ec_category` (
  `nCategoryID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '商品种类ID',
  `nLevel` int(64) DEFAULT '1' COMMENT '几级类目，从一级类目开始',
  `nParentCategoryID` bigint(64) DEFAULT NULL COMMENT '当前商品类目所在的父类目，如果为0表示当前为一级类目。',
  `sCategoryName` varchar(64) DEFAULT NULL COMMENT '商品种类',
  `sCode` varchar(32) DEFAULT '' COMMENT '商品分类编码',
  `IsLeaf` char(1) DEFAULT '0' COMMENT '是否叶子节点: 0:否 ;1:是',
  `dCreateTime` datetime DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `dUpdateTime` datetime DEFAULT CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`nCategoryID`),
  KEY `PRIMARY_nCategoryID` (`nCategoryID`) USING BTREE COMMENT '使用分类id作为主键索引'
) ENGINE=InnoDB AUTO_INCREMENT=104 DEFAULT CHARSET=utf8 COMMENT='商品分类';

-- ----------------------------
-- Table structure for r_ec_deliveryinfo
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_deliveryinfo`;
CREATE TABLE `r_ec_deliveryinfo` (
  `nDeliveryID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '配送编号',
  `sExpressCompany` varchar(32) NOT NULL DEFAULT '顺丰' COMMENT '配送公司',
  `nDeliveryPrice` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '配送价格',
  `cStatus` smallint(6) NOT NULL DEFAULT '1' COMMENT '配送状态:0:未收件；1：已收件;2：在途;3：待签收;4：已签收',
  `dCreateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '快递单创建时间',
  `dUpdateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '快递单更新时间',
  `dOutTime` datetime DEFAULT NULL COMMENT '发货时间',
  `nAddressID` bigint(64) NOT NULL DEFAULT '0' COMMENT '用户收货地址',
  `sConsignee` varchar(1024) NOT NULL DEFAULT '' COMMENT '收货人',
  `sDeliveryComment` varchar(1024) NOT NULL DEFAULT '' COMMENT '收货备注信息',
  `sDeliveryCode` varchar(64) NOT NULL DEFAULT '' COMMENT '快递单号',
  PRIMARY KEY (`nDeliveryID`),
  KEY `PRIMARY_sDeliveryID` (`nDeliveryID`) USING BTREE COMMENT ' 使用配送编号作为主键索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='配送信息';

-- ----------------------------
-- Table structure for r_ec_image
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_image`;
CREATE TABLE `r_ec_image` (
  `nImageID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '图片ID',
  `nImageType` varchar(10) NOT NULL DEFAULT '1' COMMENT '类型：1.商品图片，2.品牌图片,3.标签图片',
  `sImageName` varchar(32) NOT NULL DEFAULT '' COMMENT '附件名称',
  `dUpdateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `sDescription` varchar(255) NOT NULL DEFAULT '' COMMENT '描述',
  `sPath` varchar(255) NOT NULL DEFAULT '/path/path/path/path/image.jpg' COMMENT '图片url',
  `dCreateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`nImageID`),
  KEY `PRIMARY_nImageID` (`nImageID`) USING BTREE COMMENT '使用image的id作为主键索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='图片附件信息';

-- ----------------------------
-- Table structure for r_ec_orderinfo
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_orderinfo`;
CREATE TABLE `r_ec_orderinfo` (
  `nOrderID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '订单ID\r\n',
  `nUserID` bigint(64) NOT NULL COMMENT '下单人。\r\n关联R_EC_UserInfo表获取到需要显示的用户信息\r\n',
  `sParentOrderID` bigint(64) NOT NULL DEFAULT '0' COMMENT '父订单\r\n默认为空\r\n',
  `cPaymentMethod` smallint(6) NOT NULL DEFAULT '0' COMMENT '付款方式:0：信用支付;1：货到付款;2：银行卡支付;3：白条',
  `nDiscount` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '促销折扣',
  `nTotalQuantity` int(64) NOT NULL DEFAULT '0' COMMENT '总数量',
  `nTotalPrice` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '总价',
  `cStatus` smallint(6) NOT NULL DEFAULT '0' COMMENT '订单状态:0：未支付;1：已支付未发货;2：已发货;3：已接收;4：已关闭\r\n',
  `sCreateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '订单创建时间\r\n格式:YYYY-MM-DD HH:MM:SS\r\n',
  `sUpdateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '订单修改时间\r\n格式:YYYY-MM-DD HH:MM:SS\r\n',
  `sCompletedTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '订单完成时间',
  `sCustomerMark` varchar(1024) NOT NULL DEFAULT '' COMMENT '用户留言',
  `nDeliveryID` bigint(64) NOT NULL DEFAULT '0' COMMENT '配送编号。\r\n定义为索引\r\n',
  `sOrderCode` char(32) NOT NULL DEFAULT '' COMMENT '订单编号,由系统自动生成，生成订单编号时包含日期.',
  `sShopCode` char(32) NOT NULL DEFAULT '' COMMENT '商铺编码',
  `sOrderType` smallint(6) NOT NULL DEFAULT '0' COMMENT '订单类型,0普通订单,1合并订单,2分拆订单,3换货单,4秒杀订单,5预购订单,6限时抢购订单,7大客户订单,8订单，9合约机，10代销,11B2XB订单,12B2DC订单',
  `dPaymentTime` datetime DEFAULT NULL COMMENT '订单支付时间',
  `sOrderSource` smallint(6) NOT NULL DEFAULT '1' COMMENT '订单来源:1商城,2企业通道,3商城,4手机客户端,5分销订单',
  `nAddressID` bigint(64) NOT NULL DEFAULT '0' COMMENT '用户收货地址',
  PRIMARY KEY (`nOrderID`),
  KEY `PRIMAEY_sOrderID` (`nOrderID`) USING BTREE COMMENT '使用订单id作为主键索引',
  KEY `PRIMARY_nUserID` (`nUserID`) USING BTREE COMMENT '使用用户id作为主键索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='订单信息';

-- ----------------------------
-- Table structure for r_ec_ordersku
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_ordersku`;
CREATE TABLE `r_ec_ordersku` (
  `nOrderSKUID` bigint(64) NOT NULL AUTO_INCREMENT,
  `nOrderID` bigint(64) NOT NULL COMMENT '订单ID',
  `nSKUID` bigint(64) NOT NULL DEFAULT '0' COMMENT '商品ID',
  `nQuantity` bigint(64) NOT NULL DEFAULT '0' COMMENT '购买数量',
  `nOrigPrice` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '商品原价',
  `nDiscount` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '折扣',
  `sCurrency` varchar(16) NOT NULL DEFAULT 'RMB' COMMENT '货币类型:RMB',
  `nPrice` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '销售价格',
  PRIMARY KEY (`nOrderSKUID`),
  KEY `PRIMARY_sOrderID` (`nOrderID`) USING BTREE COMMENT '使用订单id作为主键索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='订单SKU';

-- ----------------------------
-- Table structure for r_ec_shoppingcart
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_shoppingcart`;
CREATE TABLE `r_ec_shoppingcart` (
  `nShoppingCartID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '购物车ID',
  `nUserID` bigint(64) NOT NULL DEFAULT '0' COMMENT ' 用户id',
  `sUpdateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '附件更新时间\r\n格式：YYYY-MM-DD HH:MM:SS\r\n',
  `nDiscount` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '折扣',
  `nTotalQuantity` bigint(64) NOT NULL DEFAULT '0' COMMENT '总数量',
  `sCurrency` varchar(16) NOT NULL DEFAULT 'RMB' COMMENT '货币类型',
  `nTotalPrice` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '总价',
  `nCreateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  PRIMARY KEY (`nShoppingCartID`),
  KEY `PRIMARY_nUserID` (`nUserID`) USING BTREE COMMENT '使用用户id作为主键索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='购物车';

-- ----------------------------
-- Table structure for r_ec_sku
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_sku`;
CREATE TABLE `r_ec_sku` (
  `nSKUID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '商品id',
  `nSPUID` bigint(64) NOT NULL DEFAULT '0' COMMENT 'SPU ID 默认为0 即没有商品',
  `nColor` varchar(16) NOT NULL DEFAULT '白色' COMMENT '颜色',
  `sSize` varchar(512) NOT NULL DEFAULT '4GB+128GB' COMMENT '尺寸 规格 制式 容量 套餐',
  `nPrice` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '商品价格',
  `nDisplayPrice` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '商品展示价格',
  `nInventory` int(64) NOT NULL DEFAULT '0' COMMENT '库存数量',
  `nDiscount` decimal(10,2) NOT NULL DEFAULT '0.00' COMMENT '折扣',
  `sCurrency` varchar(16) NOT NULL DEFAULT 'RMB' COMMENT '货币类型:RMB...',
  `dCreateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `dUpdateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`nSKUID`),
  KEY `PRIMARY_nProductID` (`nSKUID`) USING BTREE COMMENT '使用产品id作为主键索引'
) ENGINE=InnoDB AUTO_INCREMENT=19372 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for r_ec_spu
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_spu`;
CREATE TABLE `r_ec_spu` (
  `nSPUID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '商品ID',
  `sSPUBriefName` varchar(50) NOT NULL DEFAULT '' COMMENT ' 简称',
  `sSPUName` varchar(64) NOT NULL DEFAULT '' COMMENT '商品名称',
  `sMetaKeywords` varchar(255) NOT NULL DEFAULT '' COMMENT '商品关键字用于模糊检索',
  `nCategoryID` bigint(64) NOT NULL DEFAULT '0' COMMENT '商品种类ID,值为0表示未进行分类',
  `nBrandID` bigint(64) NOT NULL DEFAULT '0' COMMENT '品牌ID,值为0表示没有品牌',
  `nImageID` bigint(64) NOT NULL DEFAULT '0' COMMENT 'Image ID,,值为0表示没有展示图片',
  `sDescription` varchar(2000) NOT NULL DEFAULT '' COMMENT '描述信息',
  `sCode` varchar(20) NOT NULL DEFAULT '' COMMENT '商品编码',
  `sIsVirtual` char(1) NOT NULL DEFAULT '0' COMMENT '是否为虚拟商品: 0:实物商品;1:虚拟商品',
  `dCreateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `dUpdateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '修改时间',
  PRIMARY KEY (`nSPUID`),
  KEY `PRIMARY_nProductID` (`nSPUID`) USING BTREE COMMENT ' 使用产品id作为主键索引'
) ENGINE=InnoDB AUTO_INCREMENT=644 DEFAULT CHARSET=utf8 COMMENT='商品信息';

-- ----------------------------
-- Table structure for r_ec_userbankinfo
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_userbankinfo`;
CREATE TABLE `r_ec_userbankinfo` (
  `sCardID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT ' 银行卡号',
  `nUserID` bigint(64) NOT NULL COMMENT '用户id',
  `sBank` smallint(6) NOT NULL DEFAULT '0' COMMENT '绑定卡对应的银行。0：中国银行;1：农业银行;2：工商银行;3：建设银行...',
  `sCardType` smallint(6) NOT NULL DEFAULT '1' COMMENT '银行卡种类。1：借记卡;2：信用卡',
  `sCurrency` varchar(16) NOT NULL DEFAULT 'RMB' COMMENT '货币种类',
  `sCardNumber` varchar(32) NOT NULL DEFAULT '' COMMENT '银行卡号',
  PRIMARY KEY (`sCardID`),
  KEY `PRIMARY_nUserID` (`nUserID`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='用户银行卡信息';

-- ----------------------------
-- Table structure for r_ec_userdeliveryaddress
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_userdeliveryaddress`;
CREATE TABLE `r_ec_userdeliveryaddress` (
  `nAddressID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '用户地址编号',
  `nUserID` bigint(64) NOT NULL COMMENT '用户ID',
  `sFirstName` varchar(32) NOT NULL DEFAULT '姓' COMMENT '收件人First Name',
  `sLastName` varchar(32) NOT NULL DEFAULT '名' COMMENT '收件人Last Name',
  `sAddress` varchar(255) NOT NULL COMMENT '详细收货地址，街道、门牌号、小区',
  `sCity` varchar(16) NOT NULL DEFAULT '上海' COMMENT '城市',
  `sProvince` varchar(16) NOT NULL DEFAULT '上海' COMMENT '省份',
  `sCountry` varchar(16) NOT NULL DEFAULT '中国' COMMENT '国家',
  `sEmail` varchar(32) NOT NULL DEFAULT 'example@huawei.com' COMMENT '收件人邮箱地址',
  `sPhoneNo` varchar(32) NOT NULL DEFAULT '00000000000' COMMENT '收货地手机号',
  `sDistrict` varchar(32) NOT NULL DEFAULT '浦东新区' COMMENT '区',
  `dCreateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `dUpdateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '更新时间',
  `sZipCode` varchar(32) NOT NULL DEFAULT '' COMMENT '邮编',
  PRIMARY KEY (`nAddressID`),
  KEY `PRIMARY_nUserID` (`nUserID`) USING BTREE COMMENT '使用用户id作为主键索引',
  KEY `PRIMARY_sEmail` (`sEmail`) USING BTREE COMMENT '使用用户email作为主键索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='用户配送地址';

-- ----------------------------
-- Table structure for r_ec_userinfo
-- ----------------------------
DROP TABLE IF EXISTS `r_ec_userinfo`;
CREATE TABLE `r_ec_userinfo` (
  `nUserID` bigint(64) NOT NULL AUTO_INCREMENT COMMENT '用户ID，由系统自动生成',
  `sLoginName` varchar(32) NOT NULL DEFAULT '' COMMENT '登陆名,不能为空，并且不能重复',
  `sLoginPassword` varchar(32) NOT NULL DEFAULT '' COMMENT '登陆密码\r\n密码需要加密后保存\r\n',
  `sFirstName` varchar(32) NOT NULL DEFAULT '姓' COMMENT '用户First Name',
  `sLastName` varchar(32) NOT NULL DEFAULT '名' COMMENT '用户Last Name',
  `sPhoneNo` varchar(16) NOT NULL DEFAULT '13800138000' COMMENT '手机号码\r\n格式：13800138000\r\n       (021)12345678\r\n       (0086)13800138000\r\n',
  `sEmailAddress` varchar(32) NOT NULL DEFAULT 'example@huawei.com' COMMENT '电子邮箱',
  `cGender` smallint(6) NOT NULL DEFAULT '1' COMMENT '性别\r\n0：男\r\n1：女\r\n2：其它\r\n',
  `sBirthday` date NOT NULL DEFAULT '1970-00-00' COMMENT '生日\r\n格式：YYYY-MM-DD\r\n',
  `sRegisterTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间\r\n格式：YYYY-MM-DD HH:MM:SS\r\n',
  `sPayPassword` varchar(32) NOT NULL DEFAULT '' COMMENT '支付密码。\r\n保存时需要加密保存\r\n',
  PRIMARY KEY (`nUserID`),
  UNIQUE KEY `UNIQUE_sLoginName` (`sLoginName`) USING BTREE COMMENT '此唯一索引主要保证用户名的唯一性',
  KEY `PRIMARY_nUserID` (`nUserID`) USING BTREE COMMENT '使用用户id作为主键索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='用户基本信息表';
SET FOREIGN_KEY_CHECKS=1;
