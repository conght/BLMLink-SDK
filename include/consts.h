//
//  consts.h
//  blmcore
//
//  Created by guo fly on 12-11-23.
//  Copyright (c) 2012年 dodobird. All rights reserved.
//


#define MSG_CODEKEY				"eid"
#define MSG_DESKEY				"emsg"

//the same with the server response code
#define MSG_SUCC				0
#define MSG_INVALID				1
#define MSG_FAIL				2
#define MSG_DBEXCEPTION			3

//the local client code
#define MSG_PARSEERROR			1000


#define MAKECMD(mt,st) ((st & 0xff) | ((mt & 0xff) << 8))