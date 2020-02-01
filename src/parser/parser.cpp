#include "parser.hpp"

#include "env.hpp"
#include "log.hpp"

#include "../handler.hpp"

namespace parser
{
    // --- IMPORTANT SECTION ---
    node get_dtype()
    {
        node d("data_type");

        token t = look_now();
        if(match(dtype))
        {
            if(t.value == "int")
            {
                d.push(node("int"));
            }
            else if(t.value == "char")
            {
                d.push(node("char"));
            }
            else if(t.value == "bool")
            {
                d.push(node("bool"));
            }
            else if(t.value == "byte")
            {
                d.push(node("byte"));
            }
            else
            {
                d.push(node("object??"));
            }
        }
        else
        {
            // error or something
        }

        return d;
    }

    std::string get_math_op(std::string symbol)
    {
        std::string res;

        if(symbol == "+")
        {
            res = "add";
        }
        else if(symbol == "-")
        {
            res = "sub";
        }
        else if(symbol == "*")
        {
            res = "multi";
        }
        else if(symbol == "/")
        {
            res = "div";
        }
        else
        {
            res = "unknown";
            // Error or something
            handler::acc_error("Unrecognized math operator");
        }

        return res;
    }

    std::string get_conditional_op(std::string sym)
    {
        if(sym == "==")
        {
            return "equals";
        }
        else
        {
            // error
            return "idk";
        }
    }

    node get_operand()
    {
        node o;

        std::string txt = look_now().value;

        if(match(ident))
        {
            o = node("identifier");
        }
        else if(match(literal))
        {
            o = node("literal");
        }

        o.push(node(txt));

        return o;
    }

    node get_condition()
    {
        node c;
        node op = get_operand();

        // Comparaisons
        if(match(cond_op))
        {
            c.value = get_conditional_op(look_back().value);

            c.push(op);
            c.push(get_operand());
        }

        // Booleans values

        return c;
    }

    node get_expr()
    {
        node e;

        node op = get_operand();
        //e.push(get_operand());

        if(match(math_op))
        {  
            e.value = get_math_op(look_back().value);

            e.push(op);
            e.push(get_expr());
        }
        else
        {
            e = op;
        }

        return e;
    }
    // --- END OF THE USEFULL SECTION ---
    
    // --- IS SECTION ---
    bool is_entry()
    {
        mark_location();

        if(match(entry))
        {
            go_back();
            return true;
        }

        go_back();
        return false;
    }

    // Return true if is function declaration
    bool is_func()
    {
        mark_location();

        if(match(dtype) && match(ident))
        {
            go_back();
            return true;
        }
        go_back();
        return false;
    }

    // Return true if variable declaration
    bool is_decl()
    {
        mark_location();

        if(match(dtype) && match(confirm) && match(ident))
        {
            log(debug,"Is a decl");
            go_back();
            return true;
        }
        go_back();
        return false;
    }

    bool is_assign()
    {
        mark_location();

        if(match(ident) && match(assign))
        {
            log(debug,"is an assign");
            go_back();
            return true;
        }

        go_back();
        return false;
    }

    bool is_call()
    {
        mark_location();

        if(match(ident) && match(lparen))
        {
            log(debug,"is a function call");
            go_back();
            return true;
        }

        go_back();
        return false;
    }
    // --- END OF THE IS SECTION ---

    node get_assign()
    {
        node a("assignment");

        std::string i = look_now().value;
        
        expect(ident);
        expect(assign);

        a.push(node("identifier")).push(i);
        a.push(node("expression")).push(get_expr());

        return a;
    }

    node get_decl()
    {
        node d("declaration");

        log(debug,"Start declaration");

        d.push(get_dtype()); 
        expect(confirm);

        d.push(node("identifier"));
        d.get("identifier").push(node(look_now().value));
        expect(ident); // Should make custom error for that

        back();

        if(is_assign())
        {
            d.push(get_assign());
            //node exp("expression");
            //exp.push(get_expr());
            //d.push(exp);
        }
        else
        {
            next();
        }

        log(debug,"Finished declaration");
        //log(debug,d.dump());

        return d;
    }

    node get_params()
    {
        node n("params");

        expect(lparen);

        while(!match(rparen))
        {
            n.push(get_decl());
            match(comma);
            //expect(comma);
            log(debug,"Parameter done");
        }

        return n;
    }

    node get_args()
    {
        node n;

        while(!match(rparen))
        {
            n.push(get_expr());
            expect(comma);
        }

        return n;
    }

    // Compound statement
    node get_compound_stmt()
    {
        log(debug,"Started compound");

        node b("compound");
        
        expect(lbrace);

        while(!match(rbrace))
        {
            b.push(get_stmt());
        }

        log(debug,"Finished compound");

        return b;
    }

    node get_call()
    {
        node n;

        std::string id = look_now().value;
        expect(ident);
        n.push(node("identifier")).push(id);

        return n;
    }

    node get_ret()
    {
        node n("return");

        expect(ret_kw);
        n.push(node("expression")).push(get_expr());

        return n;
    }

    // Returns a expression statement,
    // such as function calls, assignements
    node get_expr_stmt()
    {
        node n;

        if(is_call())
        {
            n = get_call();
        }
        else if(is_decl())
        {
            n = get_decl();
        }
        else if(is_assign())
        {
            n = get_assign();
        }
        else if(look_now().type == ret_kw)
        {
            n = get_ret();
        }

        return n;
    }

    // --- Conditions ---

    node get_if_stmt()
    {
        node i("if");

        expect(if_kw);
        //expect(lparen);

        node c("condition");

        c.push(get_condition());
        i.push(get_compound_stmt());
        
        if(match(else_kw)) {}

        i.push(c);

        return i;
    }

    node get_while_stmt()
    {
        node w("while");

        expect(while_kw);

        node c("condition");
        c.push(get_condition());
        w.push(get_compound_stmt());

        c.push(c);

        return w;
    }

    node get_for_stmt()
    {
        node f("for");

        return f;
    }

    node get_import()
    {
        node i("import");

        expect(import_kw);
        i.push(look_now().value);
        expect(literal);

        return i;
    }

    node get_func()
    {
        node f("function");
        log(debug,"Getting function");

        std::string dt = look_now().value;
        expect(dtype);

        std::string id = look_now().value;
        expect(ident);

        f.push(node("data_type")).push(node(dt));
        f.push(node("identifier")).push(node(id));
        f.push(get_params());
        f.push(get_compound_stmt());

        return f;
    }

    // --- statements ---

    node get_stmt()
    {
        node s;

        token_type t = look_now().type;

        switch(t)
        {
            case if_kw:
            // log(debug,"Got if keyword");
            s = get_if_stmt();
            break;

            case while_kw:
            s = get_while_stmt();
            break;

            case for_kw:
            s = get_for_stmt();
            break;

            default:
            //log(debug,"Getting stmt with tkn: " + look_now().value);
                if(is_decl())
                {
                    log(debug,"Found declaration in get_Stmt()");
                    s = get_decl();
                }
                else
                {
                    s = get_expr_stmt();   
                }
            break;
        }

        return s;
    }

    node get_entry()
    {
        node e("entry");
        expect(entry);

        log(debug,"get_compound statement for entry");

        node block = get_compound_stmt();
        e.push(block);

        return e;
    }

    std::vector<node> run()
    {
        std::vector<node> res;

        while(!match(eof))
        {
            /* 
             Top Level
             Current can only have global variable
             declarations, or function definitions
            */
            if(is_func())
            {
                // get function
                res.push_back(get_func());
                log(debug,"Found function on top-level");
            }
            else if(is_decl())
            {
                log(debug,"Found var decl on top-level");
                res.push_back(get_decl());
            }
            else if(is_entry())
            {
                log(debug,"Is entry");
                res.push_back(get_entry());
            }
            else if(look_now().type == import_kw)
            {
                res.push_back(get_import());
            }
            else
            {
                // error
                //log(warning,"Unrecognized statement on top-level");
            }
        }

        log(debug,"Dumping parse tree");
        log(debug,"------------------");

        for(int i = 0; i < res.size(); i++)
        {
            log(debug,res.at(i).dump_xml());
            log(debug,"---=====---");
        }

        return res;
    }
}