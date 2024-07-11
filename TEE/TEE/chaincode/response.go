package main

import (
	"encoding/json"
	"errors"
)

const (
	// 成功
	OK = "200"

	//数据已存在
	RoundError = "1"

	//数据不存在
	StateError = "2"

	//验证错误
	VerifyError = "3"

	//数据不存在
	DataNotExist = "4"

	//未定义错误
	UnKnow = "99"
)

type Response struct {
	Code    string `json:"code"`    //返回码
	Message string `json:"message"` //返回消息
}

func NewErrorResponse(code, message string) error {
	res := Response{
		Code:    code,
		Message: message,
	}
	resAsBytes, _ := json.Marshal(res)

	return errors.New(string(resAsBytes))
}

func RoundDifferent(message string) error {
	return NewErrorResponse(RoundError, message)
}

func StateDifferent(message string) error {
	return NewErrorResponse(StateError, message)
}

func NewDataNotExistError(message string) error {
	return NewErrorResponse(DataNotExist, message)
}

func TransferError(err error) error {
	return NewErrorResponse(UnKnow, err.Error())
}

func VerifyWrong(err error) error {
	return NewErrorResponse(VerifyError, err.Error())
}

func NewOKResponse() string {
	res := Response{
		Code:    OK,
		Message: "",
	}
	resAsBytes, _ := json.Marshal(res)
	return string(resAsBytes)
}
