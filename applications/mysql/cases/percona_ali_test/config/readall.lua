pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
    set_vars()
end

function event(thread_id)
    local table_name
    table_name = "sbtest".. oltp_tables_count
    oltp_table_size = oltp_table_size + 10
    rs = db_query("SELECT * FROM ".. table_name .." WHERE id <" .. oltp_table_size)
end

