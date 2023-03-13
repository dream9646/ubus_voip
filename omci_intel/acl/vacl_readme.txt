                             VACL readme
                             ===========

Author: light
Date: 2014-02-06 19:12:37 CST


Table of Contents
=================
1 第一部份 gpon_monitor CLI
    1.1 軟體部份
        1.1.1 vacl初始化(init)
        1.1.2 vacl規則的設定(set)
        1.1.3 vacl規則的加入(add)、刪除(del)、有效(valid)、列出所有(dump)、確認(commit)
    1.2 硬體部份
        1.2.1 vacl硬體初始化(hw_init)
        1.2.2 硬體確認(hw_commit)
        1.2.3 硬體列出所有(hw_dump)
2 第二部份 VACL API
    2.1 軟體部份
        2.1.1 初始化
        2.1.2 全域設定
        2.1.3 acl規則的設定
        2.1.4 acl規則的加入(add)、刪除(del crc,hworder,order)、列出所有(dump)、確認(commit)
    2.2 硬體部份
        2.2.1 acl硬體初始化(hw_init)
        2.2.2 硬體確認 vacl_hw_g.commit()
        2.2.3 硬體列出所有 vacl_hw_g.dump()
3 第三部份 VACL CLI & API 相同特性
    3.1 比對條件的欄位分成三個群組。比對條件不能跨越群組。
        3.1.1 第一個群組：
        3.1.2 第二個群組：
        3.1.3 第三個群組：
    3.2 硬體總規則數量為64條
        3.2.1 硬體 meter 總規則數量為32條
4 第四部份 VACL CLI test case
5 第五部份 VACL API sample code


1 第一部份 gpon_monitor CLI 
============================

1.1 軟體部份 
-------------

1.1.1 vacl初始化(init) 
~~~~~~~~~~~~~~~~~~~~~~~
* 任何時候都可以使用初始化命令將軟體部份的環境清理乾淨並完成初始化。 
  
  
  vacl init
  

1.1.2 vacl規則的設定(set) 
~~~~~~~~~~~~~~~~~~~~~~~~~~
* 每一條vacl規則包含：a.比對條件(field)、b.執行動作(action)、c.有效埠(port)、d.排序值(order)、e.care tag。 
* 請使用set命令來設定這五部份的細節資訊。 
  + 此部份線上幫助解說命令為vacl set help。 
    - a.比對條件(field) 
      
      
      vacl set fld [dmac | dmac_m | smac | smac_m] [$string]
      vacl set fld [stag | ctag] [$tag_val] [$tag_mask]
      vacl set fld [sip | sip_m | dip | dip_m] [$string]
      vacl set fld [sip6 | sip6_m | dip6 | dip6_m] [$string]
      vacl set fld [sip_lb | sip_ub | dip_lb | dip_ub] [$string]
      vacl set fld [sip6_lb | sip6_ub | dip6_lb | dip6_ub] [$string]
      vacl set fld [cvid_lb | cvid_ub | svid_lb | svid_ub] [$number]
      vacl set fld [sport_lb | sport_ub | dport_lb | dport_ub] [$number]
      vacl set fld [pktlen_lb | pktlen_ub | pktlen_invert] [$number]
      vacl set fld [gem | gem_m] [$number]
      vacl set fld [ethertype(et) | ethertype_m(et_m)] [$number]
      vacl set fld [ethertype_lb(et_lb) | ethertype_ub(et_ub)] [$number]
      
    - b.執行動作(action) 
      
      
      vacl set act [0..6] for 0_Drop FWD(1_Copy 2_Redirect 3_Mirror) 4_Trap-to-CPU 5_Meter 6_Latch
      vacl set act [730-732] for 730_pri-ipri, 731_pri-dscp, 732_pri-dot1p
      vacl set act port [$forward_port_mask], logical portmask, p5,p4,...,p1,p0
      vacl set act meter [$burst] [$rate] [$ifg]
      vacl set act meter [$burst] [$rate] [$ifg] index [$table_index]
      
    - c.有效埠(port) 
      
      
      vacl set port  [$port_mask], logical portmask, p5,p4,...,p1,p0
      
    - d.排序值(order) 
      
      
      vacl set order [$number], user define rule order 0~255
      
    - e.care tag 
      
      
      vacl set care_tag(ct) [$var], $var=ctag,stag,ipv4,ipv6,pppoe,tcp,udp
      
* 比對條件(field) 
  + 這部份命令是以 vacl set fld 為開頭的命令。 
  + ether type range的設定屬於例外 
    - 因為硬體本身並沒有提供這項功能，所以用ether type mask來模擬之。 
    - 所輸入的ether type range rule在vacl add時會被拆成數個ether type mask rules來取代原來的range rule。接下來的行為就跟一般輸入ehter type mask rule無異。 
      
* 執行動作(action) 
  + 這部份命令是以vacl set act"為開頭的命令。 
  + 支援的型態為： 
    - Drop(0) 
    - Forward Copy(1), Forward Redirect(2), Forward Mirror(3), 
    - Trap-to-CPU(4) 
    - Meter(5) 
    - Latch(6) 
    - pri-ipri(730) 
    - pri-dscp(731) 
    - pri-dot1p(732) 
  + 其中三種Forward需要另外設定logical port mask。 
  + meter相關： 
    - meter需要另外設定bucket_size, rate, ifg。 
    - meter可以透過vacl單獨設定，不需要另外設定field, port_mask, order等。 
    - CLI語法為 "vacl set act meter [$burst] [$rate] [$ifg] index [$table_index]" 
      "index", "$table_index" 可有可無。$table_index=-1表示系統自行決定，否則依照使用者指定之。若該位址已經被使用，則算為錯誤。
  + Priority Assignment 
    - 730 (internal ACL Priority)需要(另外)設定priority 0~7。 
    - 731 (DSCP Remarking)需要(另外)設定DSCP 0~63。 
    - 732 (dot1p Remarking)需要(另外)設定priority 0~7。 
  + 此部份線上CLI幫助解說命令為vacl set act help。 
    
    
    vacl set act 0, for Drop
    vacl set act 1, for Forward Copy
    vacl set act 2, for Forward Redirect
    vacl set act 3, for Forward Mirror
    vacl set act        port  [$port_mask], used for action FWD(Copy, Redirect, Mirror) logical portmask
    vacl set act 4, for Trap-to-CPU
    vacl set act 5, for Meter
    vacl set act        meter [$bucket_size] [$rate] [$ifg_include]
                              [$bucket_size] <0~65535>, byte
                              [$rate]        <1~131071>,1 means 64Kbps, 2 128Kbps, ...
                              [$ifg_include] <0~1>
    vacl set act 6, for Latch
    vacl set act 730  [0~7],  internal ACL priority bits assignment
    vacl set act 731  [0~63], DSCP remarking priority assignment of IP packet
    vacl set act 732  [0~7],  dot1p remarking priority assignment of VLAN tagged packet
    
* 規則之有效埠口組(port_mask) 
  + 請使用vacl set port"來設定該vacl規則的有效埠口組。 
  + 該值為邏輯埠口組(logical port mask)並非實體的(physical port mask)。 
  + 有效埠口組值0x01代表埠口0有效，0x02代表埠口1有效，0x03代表埠口0和1有效，依此類推。 
    
* 排序值(order) 
  + 請使用vacl set order"來設定vacl規則的使用者預定排序值。 
  + 排序值的有效範圍為0~255，其中0~99為保留給classification使用。 
  + 不同的排序值以越小的值為越優先。 
  + 相同排序值的規則之先後順序依照規則被加入的先後順序而定。 
    

1.1.3 vacl規則的加入(add)、刪除(del)、有效(valid)、列出所有(dump)、確認(commit) 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* 設定完一條vacl規則之後，請用加入(add)命令來將這一條vacl規則加入到規則池(rule pool)。 
* 或者使用刪除(del)命令來將這一條vacl規則從規則池當中刪除掉，刪除之後必須再使用硬體確認(hw_commit)命令來使其生效。 
  + vacl del all, 為刪除軟硬體所有規則 
  + 此部份線上CLI幫助解說命令為vacl del help。 
    
    
    vacl del all
    vacl del [hw_order | ho] [$order]
    vacl del crc [$crc]
    vacl del order [$order]
    
* 加入及刪除的動作可以相互交叉以及重複進行多次。 
* 有效(valid)的命令目的在於使規則生效或失效，而不需要刪除或再加入該規則。 
* 規則的設定過程當中，可以利用列出所有(dump)命令來將規則池所有規則的細節一一列出，以便檢查設定完整與否。但這時不會包含規則的硬體排序值(hw order)。 
* 一旦所有規則已經設定加入完成，並且檢查完規則池所有規則後，就可以使用確認(commit)命令來產生規則池中所有規則的硬體排序值。 
* 確認(commit)命令並非強制，也可以直接執行硬體確認(hcommit)。硬體確認(hcommit)的動作會自動產生硬體排序值(hw order)。 
* 使用確認命令產生規則的硬體排序值之後，也可以再列出規則池所有規則，來檢查每一條規則的所有細節，這時就會包含硬體排序值了。 
  

1.2 硬體部份 
-------------

1.2.1 vacl硬體初始化(hw_init) 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* 建議在系統初始時就完成初始化。 
  
  
  vacl hw_init
  

1.2.2 硬體確認(hw_commit) 
~~~~~~~~~~~~~~~~~~~~~~~~~~
* 使用硬體確認(hw_commit)命令來將軟體部份的規則池中所有的規則放到硬體裡面，並使規則在硬體中發生效用。 
  

1.2.3 硬體列出所有(hw_dump) 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* 使用硬體列出所有(hw_dump)命令來將硬體規則池所有規則的細節一一列出，以便檢查設定完整與否。 
  
  

2 第二部份 VACL API 
====================

2.1 軟體部份 
-------------

2.1.1 初始化 
~~~~~~~~~~~~~


  vacl_hw_register(NULL);         // hw functions dummy hooking
  vacl_init();                    // initialization

2.1.2 全域設定 
~~~~~~~~~~~~~~~


  vacl_mode_set(mode);            // optional, set rules mode, mode=64 or 128. default is 64
  vacl_port_enable_set(0x3f);     // optional, 設定所有acl規則的全域邏輯有效埠。 default is 0x3f
  // 有效埠口值0x01代表埠口0有效，0x02代表埠口1有效，0x03代表埠口0和1有效，依此類推。
  vacl_port_permit_set(0x3f);     // optional, 設定Permit的全域邏輯有效埠，所有的封包都會被轉送，但只有吻合acl規則的封包會"執行動作"。default is 0x3f

2.1.3 acl規則的設定 
~~~~~~~~~~~~~~~~~~~~
* 資料結構初始化 
  
  
  struct vacl_user_node_t acl_rule;
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  
* 比對條件(field) 
  + 相關變數為 "external use variables" 部份的 "each rule field variables"。 
    - 參考 vacl.h 的 struct vacl_user_node_t 資料結構。 
      
      
      unsigned char dstmac_val[6], dstmac_mask[6];
      unsigned char srcmac_val[6], srcmac_mask[6];
      union {
              struct acl_uint16_bound_t bound;
              struct acl_uint16_value_t vid;
      } ctag_u;
      union {
              struct acl_uint16_bound_t bound;
              struct acl_uint16_value_t vid;
      } stag_u;
      union {
              struct acl_ipaddr_bound_t bound;
              struct acl_ipaddr_value_t addr;
      } dip_u;
      union {
              struct acl_ipaddr_bound_t bound;
              struct acl_ipaddr_value_t addr;
      } sip_u;
      union {
              struct acl_ip6addr_bound_t bound;
              struct acl_ip6addr_value_t addr;
      } dip6_u; /* for IPv6 the address only specify IPv6[31:0] */
      union {
              struct acl_ip6addr_bound_t bound;
              struct acl_ip6addr_value_t addr;
      } sip6_u; /* for IPv6 the address only specify IPv6[31:0] */
      union {
              unsigned short  value;
              struct acl_uint16_bound_t bound;
      } dport_u;
      union {
              unsigned short  value;
              struct acl_uint16_bound_t bound;
      } sport_u;
      struct acl_uint16_bound_t pktlen_bound;
      union {
              struct acl_uint16_bound_t bound;
              struct acl_uint16_value_t etype;
      } ether_type_u;
      unsigned short  gem_llid_val;
      unsigned short  gem_llid_mask;
      unsigned char   act_meter_ifg:1;
      unsigned char   pktlen_invert:1;
      unsigned char   valid:1;
      
  + 當某條件變數給定值時，請將該條件變數的相關care_bit設定起來。 
    - 參考 vacl.h struct acl_template_field_care_t 
      
      
      struct acl_template_field_care_t
      {
      //acl pre-define template 0
              unsigned char care_dstmac_val:1;
              unsigned char care_dstmac_mask:1;
              unsigned char care_stag_val:1;
              unsigned char care_stag_mask:1;
              unsigned char care_srcmac_val:1;
              unsigned char care_srcmac_mask:1;
              unsigned char care_ether_type_val:1;
              unsigned char care_ether_type_mask:1;
      //<<< byte MSB 1st
      //acl pre-define template 1
              unsigned char care_ctag_val:1;
              unsigned char care_ctag_mask:1;
              unsigned char care_sip_val:1;
              unsigned char care_sip_mask:1;
              unsigned char care_ctag_vid_lb:1;
              unsigned char care_ctag_vid_ub:1;
              unsigned char care_stag_vid_lb:1;
              unsigned char care_stag_vid_ub:1;
      //<<< byte MSB 2nd
              unsigned char care_sip_lb:1;
              unsigned char care_sip_ub:1;
              unsigned char care_dip_lb:1;
              unsigned char care_dip_ub:1;
              unsigned char care_sip6_lb:1;
              unsigned char care_sip6_ub:1;
              unsigned char care_dip6_lb:1;
              unsigned char care_dip6_ub:1;
      //<<< byte MSB 3rd
              unsigned char care_sport_lb:1;
              unsigned char care_sport_ub:1;
              unsigned char care_dport_lb:1;
              unsigned char care_dport_ub:1;
              unsigned char care_dip_val:1;
              unsigned char care_dip_mask:1;
      //acl user define template 
              unsigned char care_dport_val:1;
              unsigned char care_sport_val:1;
      //<<< byte MSB 4th
              unsigned char care_pktlen_lb:1;
              unsigned char care_pktlen_ub:1;
              unsigned char care_dip6_val:1;
              unsigned char care_dip6_mask:1;
              unsigned char care_sip6_val:1;
              unsigned char care_sip6_mask:1;
              unsigned char care_pktlen_invert:1;
              unsigned char care_gem_llid_val:1;
      //<<< byte MSB 5th
              unsigned char care_gem_llid_mask:1;
              unsigned char care_ether_type_lb:1;
              unsigned char care_ether_type_ub:1;
      //<<< byte MSB 6th
      }__attribute__ ((packed));
      
  + 提供一些條件變數設定值的輔助函式如下，並非涵蓋所有。紀錄在vacl_util.h： 
    
    
    int vacl_dmac_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
    int vacl_smac_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
    int vacl_dip_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
    int vacl_sip_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
    int vacl_dip_bound_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub);
    int vacl_sip_bound_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub);
    int vacl_sip6_addr_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
    int vacl_dip6_bound_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub);
    int vacl_sip6_bound_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub);
    
* 執行動作(action) 
  + 相關變數為vacl.h 檔案中的 struct vacl_user_node_t{} 資料結構，"external use variables"部份的"each rule action variables"。 
    - unsigned short act_type; 
      * 值的意義為 0_Drop, FWD(1_Copy, 2_Redirect, 3_Mirror), 4_Trap-to-CPU, 5_Meter 6_Latch, PRI(7_iPri, 8_dot1p, 9_DSCP) 
      * 值的範圍設定請參照 vacl.h VACL_ACT_DROP_MASK ~ VACL_ACT_PRI_DOT1P_MASK 
    - unsigned int act_forward_portmask; 
      * for Action redirect port, logical port map 
    - meter 
      * unsigned int act_meter_bucket_size; 
        + bucket size(unit byte), <0~65535> 
      * unsigned int act_meter_rate; 
        + This value range is <1~131071> and equals to hw's <8~1048568>. The granularity of rate is 8 Kbps. 
      * unsigned char act_meter_ifg:1; 
    - priority assignment 
      * unsigned short act_pri_data; 
        
* 有效埠口(port) 
  + 相關變數為"external use variables"部份的： 
    
    
    unsigned int active_portmask; /* rule source port */
    
* 排序值(order) 
  + 相關變數為"external use variables"部份的order。 
  + 排序值的有效範圍為0~255，其中0~99為保留給classification使用。 
  + 相同排序值的規則依照加入的先後順序決定彼此間的前後關係。 
    

2.1.4 acl規則的加入(add)、刪除(del crc,hworder,order)、列出所有(dump)、確認(commit) 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


  int vacl_add(struct vacl_user_node_t *, int *);
  int vacl_del_crc(unsigned int crc, unsigned short *hw_order);
  int vacl_del_hworder(unsigned short hw_order);
  int vacl_del_order(unsigned short order, int *count);
  int vacl_commit(void);
  int vacl_dump(int fd);

* 加入(add)的方式只有循序由後加入。 
* 加入(add)函式的第二個參數回傳該規則依照先後順序加入此order序列的相對位置。當order為0而且之後沒有該規則或之前規則的刪除動作，則此位置即代表置入硬體ACL表格的規則序值。 
* 有三種刪除方式，分別依照crc, hw_order, order來刪除。 
* 呼叫刪除之後必須再執行硬體確認 vacl_hw_g.commit()，使動作生效至硬體。觀念上，刪除動作僅對vacl軟體層動作，執行硬體確認動作代表，抹掉所有硬體ACL設定，再將新的軟體規則池內容一次全部寫入硬體之中。 
  

2.2 硬體部份 
-------------

2.2.1 acl硬體初始化(hw_init) 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* 僅執行一次，每次執行後，硬體內部的規則都會被清除掉。 
  
  
  extern struct vacl_hw_t vacl_hw_fvt2510_g;
  vacl_hw_register(struct vacl_hw_t *);
  vacl_hw_g.init();
  

2.2.2 硬體確認 vacl_hw_g.commit() 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* 將所有vacl的規則一次儲存至硬體中 
  

2.2.3 硬體列出所有 vacl_hw_g.dump() 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


3 第三部份 VACL CLI & API 相同特性 
===================================

3.1 比對條件的欄位分成三個群組。比對條件不能跨越群組。 
-------------------------------------------------------

3.1.1 第一個群組： 
~~~~~~~~~~~~~~~~~~~
* destnation/source mac address 
* stag 
* ether type 
  

3.1.2 第二個群組： 
~~~~~~~~~~~~~~~~~~~
* ctag 
* ctag vid range lower/upper bound 
* stag vid range lower/upper bound 
* source/destnation ipv4 address 
* source/destnation ipv4 address lower/upper bound 
* source/destnation ipv6 address lower/upper bound, for IPv6 the address only specify IPv6[31:0] 
* layer 4 source/destnation port lower/upper bound 
  

3.1.3 第三個群組： 
~~~~~~~~~~~~~~~~~~~
* layer 4 source/destnation port 
* packet length range lower/upper bound 
* packet length range invert 
* source/destnation ipv6 address, for IPv6 the address only specify IPv6[31:0] 
* gemport id or llid 
* ether type range lower/upper bound 
  

3.2 硬體總規則數量為64條 
-------------------------

3.2.1 硬體 meter 總規則數量為32條 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


4 第四部份 VACL CLI test case 
==============================


  vacl init
  # init 會清除掉所有軟體裡面的acl規則
  vacl hw_init 
  # hinit 會清除掉所有硬體裡面的acl規則
  
  vacl set fld sip6 6780::2451:6830:2436:1476
  vacl set fld sip6_m ffff::ffff:ffff:ffff:ffff
  vacl set fld dip6 fe80::224:7eff:fede:1d7b
  vacl set fld dip6_m ffff::ffff:ffff:ffff:ffff
  vacl set fld pktlen_lb  512
  vacl set fld pktlen_ub 1518
  vacl set fld pktlen_invert 1
  vacl set act 0
  vacl set port 8
  vacl set order 30
  vacl add
  
  vacl set fld dip6 fe80::224:7eff:fede:1d7b
  vacl set fld dip6_m ffff::ffff:ffff:ffff:ffff
  vacl set act 5
  vacl set act meter 0 0 0
  vacl set port 8
  vacl set order 31
  vacl add
  
  vacl set fld dmac 00:24:7e:de:1d:7b
  vacl set fld dmac_m ff:ff:ff:ff:ff:ff
  vacl set act 0
  vacl set port 0x10
  vacl set order 32
  vacl add
  
  vacl set fld dmac ff:ff:ff:ff:ff:ff
  vacl set fld dmac_m ff:ff:ff:ff:ff:ff
  vacl set act 2
  vacl set act port 0x20
  vacl set port 0xff
  vacl set order 32
  vacl add
  
  vacl set fld smac 00:24:7e:de:1d:7b
  vacl set fld smac_m ff:ff:ff:ff:ff:ff
  vacl set act 1
  vacl set act port 15
  vacl set port 0x20
  vacl set order 33
  vacl add
  
  vacl set fld sip 192.168.1.1
  vacl set fld sip_m 255.255.255.255
  vacl set act 0
  vacl set port 8
  vacl set order 34
  vacl add
  
  vacl set fld dip 10.20.74.74
  vacl set fld dip_m 255.255.255.255
  vacl set act 0
  vacl set port 8
  vacl set order 35
  vacl add
  
  vacl set fld stag 47104 65535
  vacl set fld gem 0x6666
  vacl set fld gem_m 0xffff
  vacl set act 0
  vacl set port 8
  vacl set order 36
  vacl add
  
  vacl set fld ctag 60440 65535
  vacl set fld gem 0x7777
  vacl set fld gem_m 0xffff
  vacl set act 0
  vacl set port 8
  vacl set order 37
  vacl add
  
  vacl set fld cvid_lb 111
  vacl set fld cvid_ub 222
  vacl set fld svid_lb 333
  vacl set fld svid_ub 444
  vacl set act 0
  vacl set port 8
  vacl set order 38
  vacl add
  
  vacl set fld sport_lb  67
  vacl set fld sport_ub  68
  vacl set care_tag udp
  vacl set act 4
  vacl set port 0x2f
  vacl set order 39
  vacl add
  
  vacl set fld dip6_lb fe80::224:7eff:fede:1d7b
  vacl set fld dip6_ub fe80::224:7eff:fede:ffff
  vacl set fld sip6_lb feff::224:7eff:fede:1d7b
  vacl set fld sip6_ub feff::224:7eff:fede:ffff
  vacl set fld dip_lb 192.168.1.1
  vacl set fld dip_ub 192.168.1.255
  vacl set fld sip_lb 10.20.32.1
  vacl set fld sip_ub 10.20.32.255
  vacl set act 0
  vacl set port 8
  vacl set order 40
  vacl add
  
  vacl set fld pktlen_lb  512
  vacl set fld pktlen_ub 1518
  vacl set fld pktlen_invert 1
  vacl set act 0
  vacl set port 8
  vacl set order 41
  vacl add
  
  vacl set fld gem 0x1234
  vacl set fld gem_m 0xffff
  vacl set act 0
  vacl set port 8
  vacl set order 141
  vacl add
  
  vacl set fld ctag 60440 65535
  vacl set fld ethertype 0x0908
  vacl set fld ethertype_m 0xffff
  vacl set fld gem 0x5555
  vacl set fld gem_m 0xffff
  vacl set care_tag tcp
  vacl set act 6
  vacl set port 8
  vacl set order 142
  vacl add
  
  vacl set fld ethertype_lb 0x229f
  vacl set fld ethertype_ub 0x22a6
  vacl set act 732 3
  vacl set port 8
  vacl set order 143
  vacl add
  
  vacl set act meter 1003 1003 1 index 3
  vacl add
  
  vacl commit
  vacl dump
  
  vacl hw_commit
  vacl hw_dump
  
  vacl del all



5 第五部份 VACL API sample code 
================================


  struct vacl_user_node_t acl_rule;
  int sub_order;
  
  vacl_hw_register(NULL);         
  vacl_init();
  vacl_mode_set(64);
  vacl_port_enable_set(0x3f);
  vacl_port_permit_set(0x3f);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  vacl_smac_str_set(&acl_rule, "00:04:80:e4:53:90","ff:ff:ff:ff:ff:ff");
  acl_rule.gem_llid_val = 0x5555;
  acl_rule.gem_llid_mask = 0xffff;
  acl_rule.care_u.bit.care_gem_llid_val = 1;
  acl_rule.care_u.bit.care_gem_llid_mask = 1;
  acl_rule.act_type = VACL_ACT_DROP_MASK;
  acl_rule.active_portmask = 0x08;
  acl_rule.order = 132;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  acl_rule.act_type = VACL_ACT_METER_MASK;
  acl_rule.act_meter_bucket_size = 2048;
  acl_rule.act_meter_rate = 102400;
  acl_rule.act_meter_ifg = 1;
  acl_rule.hw_meter_table_entry = 3;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  acl_rule.stag_u.vid.value = 47104;
  acl_rule.stag_u.vid.mask = 65535;
  acl_rule.care_u.bit.care_stag_val = 1;
  acl_rule.care_u.bit.care_stag_mask = 1;
  acl_rule.act_type = VACL_ACT_FWD_COPY_MASK;
  acl_rule.act_forward_portmask = 0xf;
  acl_rule.active_portmask = 0x04;
  acl_rule.order = 32;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  acl_rule.ctag_u.bound.lower = 111;
  acl_rule.ctag_u.bound.upper = 222;
  acl_rule.stag_u.bound.lower = 333;
  acl_rule.stag_u.bound.upper = 444;
  acl_rule.care_u.bit.care_ctag_vid_lb = 1;
  acl_rule.care_u.bit.care_ctag_vid_ub = 1;
  acl_rule.care_u.bit.care_stag_vid_lb = 1;
  acl_rule.care_u.bit.care_stag_vid_ub = 1;
  acl_rule.act_type = VACL_ACT_METER_MASK;
  acl_rule.act_meter_bucket_size = 1023;
  acl_rule.act_meter_rate = 204800;
  acl_rule.act_meter_ifg = 1;
  acl_rule.active_portmask = 0x08;
  acl_rule.order = 35;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  acl_rule.sport_u.bound.lower = 5555;
  acl_rule.sport_u.bound.upper = 6666;
  acl_rule.dport_u.bound.lower = 7777;
  acl_rule.dport_u.bound.upper = 8888;
  acl_rule.care_u.bit.care_sport_lb = 1;
  acl_rule.care_u.bit.care_sport_ub = 1;
  acl_rule.care_u.bit.care_dport_lb = 1;
  acl_rule.care_u.bit.care_dport_ub = 1;
  acl_rule.care_tag_tcp_value = 1;
  acl_rule.care_tag_tcp_mask = 1;
  acl_rule.act_type = VACL_ACT_FWD_REDIRECT_MASK;
  acl_rule.act_forward_portmask = 0xf;
  acl_rule.active_portmask = 0x10;
  acl_rule.order = 36;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  vacl_dip6_bound_str_set(&acl_rule, "fe80::224:7eff:fede:1d7b", "fe80::224:7eff:fede:ffff");
  vacl_sip6_bound_str_set(&acl_rule, "fe80::224:7eff:fede:1d7b", "fe80::224:7eff:fede:ffff");
  vacl_dip_bound_str_set(&acl_rule, "192.168.1.1", "192.168.1.255");
  vacl_sip_bound_str_set(&acl_rule, "10.20.32.1", "10.20.32.255");
  acl_rule.act_type = VACL_ACT_FWD_MIRROR_MASK;
  acl_rule.act_forward_portmask = 0xf;
  acl_rule.active_portmask = 0x03;
  acl_rule.order = 37;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  acl_rule.pktlen_bound.lower = 512;
  acl_rule.pktlen_bound.upper = 1518;
  acl_rule.pktlen_invert = 1;
  acl_rule.care_u.bit.care_pktlen_lb = 1;
  acl_rule.care_u.bit.care_pktlen_ub = 1;
  acl_rule.care_u.bit.care_pktlen_invert = 1;
  acl_rule.gem_llid_val = 0x1234;
  acl_rule.gem_llid_mask = 0xffff;
  acl_rule.care_u.bit.care_gem_llid_val = 1;
  acl_rule.care_u.bit.care_gem_llid_mask = 1;
  acl_rule.act_type = VACL_ACT_FWD_MIRROR_MASK;
  acl_rule.act_forward_portmask = 0xf;
  acl_rule.active_portmask = 0x03;
  acl_rule.order = 37;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  vacl_sip6_addr_str_set(&acl_rule, "fe80::224:7eff:fede:1d7b", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
  acl_rule.act_type = VACL_ACT_LATCH_MASK;
  acl_rule.active_portmask = (1<<2);
  acl_rule.order = 139;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  acl_rule.ether_type_u.etype.value = 0x0800;
  acl_rule.ether_type_u.etype.mask = 0xffff;
  acl_rule.care_u.bit.care_ether_type_val = 1;
  acl_rule.care_u.bit.care_ether_type_mask = 1;
  acl_rule.act_type = VACL_ACT_LATCH_MASK;
  acl_rule.active_portmask = 0x07;
  acl_rule.order = 100;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  acl_rule.pktlen_bound.lower = 512;
  acl_rule.pktlen_bound.upper = 1518;
  acl_rule.pktlen_invert = 1;
  acl_rule.care_u.bit.care_pktlen_lb = 1;
  acl_rule.care_u.bit.care_pktlen_ub = 1;
  acl_rule.care_u.bit.care_pktlen_invert = 1;
  acl_rule.gem_llid_val = 0x1234;
  acl_rule.gem_llid_mask = 0xffff;
  acl_rule.care_u.bit.care_gem_llid_val = 1;
  acl_rule.care_u.bit.care_gem_llid_mask = 1;
  acl_rule.act_type = VACL_ACT_TRAP_CPU_MASK;
  acl_rule.active_portmask = 0x0f;
  acl_rule.order = 38;
  vacl_add(&acl_rule, &sub_order);
  
  memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
  vacl_user_node_init(&acl_rule);
  acl_rule.ether_type_u.etype.value = 0x0800;
  acl_rule.ether_type_u.etype.mask = 0xffff;
  acl_rule.care_u.bit.care_ether_type_val = 1;
  acl_rule.care_u.bit.care_ether_type_mask = 1;
  acl_rule.ctag_u.vid.value = 60440;
  acl_rule.ctag_u.vid.mask = 65535;
  acl_rule.care_u.bit.care_ctag_val = 1;
  acl_rule.care_u.bit.care_ctag_mask = 1;
  acl_rule.gem_llid_val = 0x1234;
  acl_rule.gem_llid_mask = 0xffff;
  acl_rule.care_u.bit.care_gem_llid_val = 1;
  acl_rule.care_u.bit.care_gem_llid_mask = 1;
  acl_rule.act_type = VACL_ACT_DROP_MASK;
  acl_rule.active_portmask = (1<<3);
  acl_rule.order = 37;
  vacl_add(&acl_rule, &sub_order);
  
  extern struct vacl_hw_t vacl_hw_fvt2510_g;
  int count = 0;
  vacl_hw_register(&vacl_hw_fvt2510_g);
  vacl_hw_g.init();
  vacl_hw_g.commit(&count);
  printf("%d rules hw_commit-ed.\n", count); 
  vacl_hw_g.dump(1);


