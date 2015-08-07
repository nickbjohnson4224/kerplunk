local kp = require('kerplunk')
local sql = require('lsqlite3')
local ffi = require('ffi')

module = {}

function module.main(sgf_path, db_path)

    -- open SGF game list
    local file = nil
    if sgf_path == nil or sgf_path == '-' then
        file = io.stdin
    else
        file = io.open(sgf_path)
    end

    -- open database
    local db = sql.open(db_path)
    if not db then
        return -1
    end

    -- create games table schema
    db:exec [[
    CREATE TABLE IF NOT EXISTS games (
        id INTEGER PRIMARY KEY,

        name VARCHAR(32),
        date DATE,
        black_name VARCHAR(32),
        black_rank VARCHAR(3),
        white_name VARCHAR(32),
        white_rank VARCHAR(3),
        copyright TEXT,
        ruleset VARCHAR(11),
        result VARCHAR(11),
        handicap TINYINT,
        num_moves INT,

        handicaps_blob_le16 BLOB,
        moves_blob_le16 BLOB
    )
    ]]

    -- prepare game insertion statement
    local stmt_insert_game = db:prepare [[
    INSERT INTO games (
        name, date,
        black_name, black_rank,
        white_name, white_rank,
        copyright, 
        ruleset,
        handicap,
        num_moves,
        handicaps_blob_le16,
        moves_blob_le16
    ) VALUES (?, date(?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ]]

    db:exec "BEGIN TRANSACTION"

    local i = 0
    while true do
        local record = kp.sgf_load(file)
        if record == nil then
            break
        end

        local function string(s)
            if s == ffi.NULL then
                return nil
            else
                return ffi.string(s)
            end
        end

        -- perform insertion of game record structure
        do
            local stmt = stmt_insert_game
            stmt:clear_bindings()
            stmt:reset()
            stmt:bind(1, string(record.name))
            stmt:bind(2, string(record.date))
            stmt:bind(3, string(record.black_name))
            stmt:bind(4, string(record.black_rank))
            stmt:bind(5, string(record.white_name))
            stmt:bind(6, string(record.white_rank))
            stmt:bind(7, string(record.copyright))
            stmt:bind(8, string(record.ruleset))        
            stmt:bind(9, tonumber(record.handicap))
            stmt:bind(10, tonumber(record.num_moves))
        
            if tonumber(record.handicap) > 0 then
                stmt:bind(11, record.handicaps, tonumber(record.handicap) * 2)
            else
                stmt:bind(11, nil)
            end

            if tonumber(record.num_moves) > 0 then
                stmt:bind(12, record.moves, tonumber(record.num_moves) * 2)
            else
                stmt:bind(12, nil)
            end
        
            stmt:step()
        end

        i = i + 1
    end

    db:exec "END TRANSACTION"

    stmt_insert_game:finalize()
    db:close()
end

return module
