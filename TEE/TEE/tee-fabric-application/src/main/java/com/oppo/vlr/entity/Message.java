package com.oppo.vlr.entity;

import lombok.AllArgsConstructor;
import lombok.Data;

@Data
@AllArgsConstructor
public class Message {
    private long id;
    private String taskID;          //日志对应的task_id
    private String localID;         //日志运行的local_id
}
