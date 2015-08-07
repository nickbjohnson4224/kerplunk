local kp = require('kerplunk')

function main(...)
    local args
    if #{...} == 0 then
        args = {'-'}
    else
        args = {...}
    end

    for i, path in ipairs(args) do
        local file = nil
        if path == nil or path == '-' then
            file = io.stdin
        else
            file = io.open(path)
        end
        
        while true do
            local record = kp.sgf_load(file)
            if record == nil then
                break
            end

            local replay = kp.new_replay(record)
            while kp.replay_step(replay) do end
            if replay.move_num == record.num_moves then 
                kp.sgf_dump(record, io.stdout)
            end
        end
    end
    return 0
end

return {main=main}
