local kp = require('kerplunk')

function main(...)
    for i, path in ipairs({...}) do
        local file = io.open(path)
        while true do
            local record = kp.sgf_load(file)
            if record == nil then
                break
            end
            kp.sgf_dump(record, io.stdout)
        end
    end
    return 0
end

return {main=main}
