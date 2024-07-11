package main

import (
	// "crypto/ecdsa"
	// "crypto/elliptic"
	// "crypto/rand"
	// "crypto/sha256"
	// "crypto/x509"
	// "encoding/pem"

	"bytes"
	"crypto/elliptic"
	"encoding/hex"
	"fmt"
	"log"
	"math/big"

	"github.com/hyperledger/fabric-contract-api-go/contractapi"
)

type tee struct {
	contractapi.Contract
}

func (tee *tee) InitLedger(ctx contractapi.TransactionContextInterface) error {
	//State = "Init"
	return nil
}

func (tee *tee) GenKey_model(taskid string, round string, syntax string, index string) string {
	return taskid + "_" + round + "_" + syntax + "_" + index
}

func (tee *tee) GenKey_pk(taskid string, syntax string) string {
	return taskid + "_" + "_" + syntax
}

func BytesCombine(pBytes ...[]byte) []byte {
	var buffer bytes.Buffer
	for index := 0; index < len(pBytes); index++ {
		buffer.Write(pBytes[index])
	}
	return buffer.Bytes()
}

//上传PK，在这一整个轮次都生效
func (tee *tee) UploadPK(ctx contractapi.TransactionContextInterface, taskid string, pk_x string, pk_y string) error {

	Pk_x_byte := []byte(pk_x)
	Pk_y_byte := []byte(pk_y)
	key_PK_x := tee.GenKey_pk(taskid, "PK_x")
	key_PK_y := tee.GenKey_pk(taskid, "PK_y")
	if err := ctx.GetStub().PutState(key_PK_x, Pk_x_byte); err != nil {
		return TransferError(err)
	}
	if err := ctx.GetStub().PutState(key_PK_y, Pk_y_byte); err != nil {
		return TransferError(err)
	}

	// State_over := "uploadPK"
	// State_byte := []byte(State_over)
	// //StateJSON, err := json.Marshal(State_over)
	// if err := ctx.GetStub().PutState(key_state, State_byte); err != nil {
	// 	return TransferError(err)
	// }
	return nil
}

func (tee *tee) GetPK(ctx contractapi.TransactionContextInterface, taskid string, syntax string) (string, error) {
	//利用genKey来生成各个参数的对应key名称
	key_PK := tee.GenKey_pk(taskid, syntax)

	// err := fmt.Errorf("Round不同")
	// if Round != round {
	// 	fmt.Print(err)
	// }
	PK_JSON, err := ctx.GetStub().GetState(key_PK)
	if err != nil {
		return "nil", TransferError(err)
	}
	PK := string(PK_JSON)
	return PK, nil
}

//B上传theta_B * x_i^B 的承诺值到链上
func (tee *tee) UploadGlobalModel(ctx contractapi.TransactionContextInterface, taskid string, round string, ciphertext string, auth_tag string, sig string, index string) error {

	// ciphertext_byte := []byte(ciphertext)
	// auth_tag_byte := []byte(auth_tag)
	// sum_byte := BytesCombine(ciphertext_byte, auth_tag_byte)
	ciphertext_byte, err := hex.DecodeString(ciphertext)
	if err != nil {
		return TransferError(err)
	}

	auth_tag_byte, err := hex.DecodeString(auth_tag)
	if err != nil {
		return TransferError(err)
	}
	if len(auth_tag_byte) == 0 {

	}

	sig_byte, err := hex.DecodeString(sig)
	if err != nil {
		return TransferError(err)
	}

	PK_x_JSON, err := ctx.GetStub().GetState(tee.GenKey_pk(taskid, "PK_x"))
	if err != nil {
		return TransferError(err)
	}
	pk_x := string(PK_x_JSON)

	PK_y_JSON, err := ctx.GetStub().GetState(tee.GenKey_pk(taskid, "PK_y"))
	if err != nil {
		return TransferError(err)
	}
	pk_y := string(PK_y_JSON)

	pk_x_big := new(big.Int)
	pk_x_big.SetString(pk_x, 16)
	pk_y_big := new(big.Int)
	pk_y_big.SetString(pk_y, 16)

	// sig_byte := decodebase64(sig)
	var r_byte []byte
	var s_byte []byte
	r_byte = make([]byte, 32)
	s_byte = make([]byte, 32)
	for i := 0; i < (len(sig_byte) / 2); i++ {
		r_byte[31-i] = sig_byte[i]
		s_byte[31-i] = sig_byte[i+32]
	}
	// for i := 0; i < (len(sig_byte) / 2); i++ {
	// 	r_byte[i] = sig_byte[i]
	// 	s_byte[i] = sig_byte[i+32]
	// }
	r := new(big.Int)
	s := new(big.Int)
	r.SetBytes(r_byte)
	s.SetBytes(s_byte)

	if EccVerify2(ciphertext_byte, r, s, pk_x_big, pk_y_big, elliptic.P256()) != true {
		return VerifyWrong(fmt.Errorf("VerifyWrong"))
	}

	// ciphertext_byte := []byte(ciphertext)
	// auth_tag_byte := []byte(auth_tag)

	key_ciphertext := tee.GenKey_model(taskid, round, "ciphertext", index)
	if err := ctx.GetStub().PutState(key_ciphertext, []byte(ciphertext)); err != nil {
		return TransferError(err)
	}

	key_auth_tag := tee.GenKey_model(taskid, round, "auth_tag", index)
	if err := ctx.GetStub().PutState(key_auth_tag, []byte(auth_tag)); err != nil {
		return TransferError(err)
	}

	return nil
}

func (tee *tee) GetCiphertext(ctx contractapi.TransactionContextInterface, taskid string, round string, index string) (string, error) {
	//利用genKey来生成各个参数的对应key名称
	key_ciphertext := tee.GenKey_model(taskid, round, "ciphertext", index)

	// err := fmt.Errorf("Round不同")
	// if Round != round {
	// 	fmt.Print(err)
	// }
	ciphertext_JSON, err := ctx.GetStub().GetState(key_ciphertext)
	if err != nil {
		return "nil", TransferError(err)
	}
	if ciphertext_JSON == nil {
		// return nil, fmt.Errorf("日志[task_id:%s,local_id:%s]不存在", task_id, local_id)
		return "", NewDataNotExistError(fmt.Sprintf("[taskID:%s,round:%s,index:%s]不存在", taskid, round, index))
	}
	ciphertext := string(ciphertext_JSON)
	return ciphertext, nil
}

func (tee *tee) GetAuth_tag(ctx contractapi.TransactionContextInterface, taskid string, round string, index string) (string, error) {
	//利用genKey来生成各个参数的对应key名称
	key_auth_tag := tee.GenKey_model(taskid, round, "auth_tag", index)

	// err := fmt.Errorf("Round不同")
	// if Round != round {
	// 	fmt.Print(err)
	// }
	auth_tag_JSON, err := ctx.GetStub().GetState(key_auth_tag)
	if err != nil {
		return "nil", TransferError(err)
	}
	if auth_tag_JSON == nil {
		// return nil, fmt.Errorf("日志[task_id:%s,local_id:%s]不存在", task_id, local_id)
		return "", NewDataNotExistError(fmt.Sprintf("[taskID:%s,round:%s,index:%s]不存在", taskid, round, index))
	}
	auth_tag := string(auth_tag_JSON)
	return auth_tag, nil
}

func main() {
	teeCode, err := contractapi.NewChaincode(&tee{})
	if err != nil {
		log.Panicf("创建tee合约错误: %v", err)
	}

	if err := teeCode.Start(); err != nil {
		log.Panicf("启动tee合约错误: %v", err)
	}
}
