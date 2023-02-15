/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,TK_NUM,
  /* TODO: Add more token types */
  TK_LT,TK_GT,TK_LE,TK_GE,TK_NQ,TK_AND,TK_OR,TK_HEX,TK_REG,
  TK_POS,TK_NEG,TK_DEREF,
};


// for the token, use enum and ascii <- char to define
// token_type and type

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
	{" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"<=", TK_LE},
  {">=", TK_GE},
  {"<", TK_LT},
  {">", TK_GT},
  //? first judge as < and > will interrupt the LE and GE
  // '<='should be detectde earlier than '<'
  {"==", TK_EQ},
  {"!=", TK_NQ},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"\\(", '('},
  {"\\)", ')'},

  {"0[xX][0-9a-fA-F]+", TK_HEX },
	{"[0-9]+", TK_NUM },
  //?
  
  {"\\$\\w+", TK_REG },
};

//
// Here is a macro define!
//

#define NR_REGEX ARRLEN(rules)
#define TOKEN_BELONG(type,types) token_belong(type,types,ARRLEN(types))

//static int unary_token[]={TK_NEG,TK_POS,TK_DEREF};
static int non_token[]={')',TK_NUM,TK_HEX,TK_REG};

bool token_belong(int type,int types[],int size)
{
  for(int i=0;i<=size;++i)
    {if (type==types[i]) return true;}
  return false;
}

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) 
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
				if (rules[i].token_type == TK_NOTYPE)	break;

				tokens[nr_token].type = rules[i].token_type;

        switch (rules[i].token_type) {
          case TK_HEX:
					case TK_NUM:
          case TK_REG:
						strncpy(tokens[nr_token].str,substr_start ,substr_len);
						tokens[nr_token].str[substr_len]='\0';
            break;
          case '*':
          case '+':
          case '-':
            if(nr_token==0 || !TOKEN_BELONG(tokens[nr_token-1].type,non_token))
            {
              switch (rules[i].token_type)
              {
              case '-':
                tokens[nr_token].type=TK_NEG;
                break;
              case '+':
                tokens[nr_token].type=TK_POS;
                break;
              case '*':
                tokens[nr_token].type=TK_DEREF;
                break;
              }
            }
        }
				nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  //printf("nr_token = %d\n",nr_token);
  return true;
}


bool check_parentheses(int p,int q){
	if (tokens[p].type=='(' && tokens[q].type == ')')
	{
		int tmp=0;
		for(int i=p;i<=q;++i)
		{
			if (tokens[i].type=='(') tmp+=1;
			if (tokens[i].type==')') tmp-=1;
			
			if (tmp==0) return i==q;
		}
	}
	return false;
}

int find_major(int p,int q){
	
	int ret = -1, par = 0, op_type = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_NUM) {
      continue;
    }
    if (tokens[i].type == '(') {
      par++;
    } else if (tokens[i].type == ')') {
      if (par == 0) {
        return -1;
      }
      par--;
    } else if (par > 0 || TOKEN_BELONG(tokens[i].type,non_token)) {
      continue;
    } else {
      int tmp_type = 0;
      // trick: without break, the code will exec the follow case contents.

      switch (tokens[i].type) {
      case TK_OR: tmp_type++;
      case TK_AND: tmp_type++;
      case TK_EQ: case TK_NQ: tmp_type++;
      case TK_LT: case TK_GT: case TK_GE: case TK_LE: tmp_type++;
      case '+': case '-': tmp_type++;
      case '*': case '/': tmp_type++;
      case TK_NEG: case TK_DEREF: case TK_POS: tmp_type++; break;
      default: return -1;
      }
      if (tmp_type >= op_type) {
        op_type = tmp_type;
        ret = i;
      }
    }
  }
  if (par != 0) return -1;
  return ret;

}

word_t eval_operand(int i,bool *flag){
  // single operand
  switch(tokens[i].type)
  {
    case TK_NUM:  return strtol(tokens[i].str,NULL,10);
    case TK_HEX:  return strtol(tokens[i].str,NULL,16);
    case TK_REG:  
    //puts(tokens[i].str);
    //?
    printf("%-15x\n",(isa_reg_str2val(tokens[i].str,flag)));
    return isa_reg_str2val(tokens[i].str,flag);
    default: 
      *flag=false;
      return 0;
  }
}

extern word_t vaddr_read(vaddr_t addr,int len);


word_t eval1(int op,word_t val,bool *flag)
{
  switch (op)
  {
  case TK_NEG:
    return -val;
  case TK_POS:
    return val;
  case TK_DEREF:
    return vaddr_read(val,8);
  default:
    *flag=false;
    return 0;
  }
}

word_t eval2(int op,word_t val1,word_t val2,bool *flag)
{
  switch (op)
  {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    // div0 error
    case '/':
    if (val2==0)
    {
    *flag = false;
    return 0;
    }	
    return val1 / val2;
    case TK_AND: return val1 && val2;
    case TK_OR: return val1 || val2;
    case TK_EQ: return val1 == val2;
    case TK_NQ: return val1 != val2;
    case TK_GT: return val1 > val2;
    case TK_LT: return val1 < val2;
    case TK_GE: return val1 >= val2;
    case TK_LE: return val1 <= val2;
    default: 
    *flag = false;
    return 0;
  }
}


word_t eval(int p,int q,bool *flag) {
	*flag=true;
  if (p > q) {
    /* Bad expression */
		*flag = false;
		return 0;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    // 1.0 version:
		//word_t ret =strtol(tokens[p].str,NULL,10);
    // new eval:
		return eval_operand(p,flag);
  }
  else if (check_parentheses(p, q)) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1,flag);
  }
  else {
    int op = find_major(p,q);
    bool flag1,flag2;
    word_t  val1 = eval(p, op - 1, &flag1);
    // old ver
		//if(!*flag) return 0;
    word_t val2 = eval(op + 1, q, &flag2);
		//if(!*flag) return 0;

    //in new ver, if left flag -> true, maybe minus
    if(!flag2)
    {
      *flag=false;
      return 0;
    }

    if(flag1)
    {
      word_t ret = eval2(tokens[op].type,val1,val2,flag);
      return ret;
    }
    else
    {
      word_t ret = eval1(tokens[op].type,val2,flag);
      return ret;
    }

    //printf("op=%d\n",op);
    //char tmp_t='+';
    //printf ("tmp=%d\n",tmp_t);
    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
			// div0 error
      case '/':
			if (val2==0)
			{
			*flag = false;
			return 0;
			}	
			return val1 / val2;
      default: assert(0);
    }
  }
	return 0;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  return eval(0,nr_token-1,success);
}
