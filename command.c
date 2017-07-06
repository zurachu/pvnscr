//=============================================================================
//  引数のカンマ読み込み
//=============================================================================
BOOL pvns_GetCamma( void )
{
	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// 空白は読み飛ばす
	if( _NOW_WORD_ == ',' ) {
		sData.sp.ptr++;
		return TRUE;	// 成功
	}
	
	return FALSE;		// 失敗
}


//=============================================================================
//  スクリプトを行末まで読み飛ばす（if文用）
//=============================================================================
void pvns_SkipToLF(void)
{
	while( _NOW_WORD_ != '\n' ) {
		if( IsKanji( _NOW_WORD_ ) ) { sData.sp.ptr++; }
			sData.sp.ptr++;
		}
}


//=============================================================================
//  変数のインデックスを返す（グローバル変数は+128）
//=============================================================================
BOOL pvns_GetIndexOfVariable( short *index )
{
	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// 空白は読み飛ばす
	if( _NOW_WORD_ == '%' ) {	// ローカル変数
		sData.sp.ptr++;
		*index = atoi( pData+sData.sp.ptr );
		if( *index < 0 || *index >= _SIZEOF_LOCAL_ ) { return NULL; }
		while( isdigit( _NOW_WORD_ ) ) { sData.sp.ptr++; }	// 数字を読み飛ばす
		return TRUE;
	}
	if( _NOW_WORD_ == '$' ) {	// グローバル変数
		sData.sp.ptr++;
		*index = atoi( pData+sData.sp.ptr );
		if( *index < 0 || *index >= _SIZEOF_GLOBAL_ ) { return NULL; }
		*index += _SIZEOF_LOCAL_;
		while( isdigit( _NOW_WORD_ ) ) { sData.sp.ptr++; }	// 数字を読み飛ばす
		return TRUE;
	}
	
	return FALSE;	// 失敗
}


//=============================================================================
//  引数の値（即値、変数）を返す
//=============================================================================
BOOL pvns_GetValueOfArgument( short *val )
{
	short i;
	
	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// 空白は読み飛ばす
	if( pvns_GetIndexOfVariable( &i ) ) {	// 変数ならばインデックスから値取得
		*val = sData.variable[i];
		return TRUE;
	}
	if( isdigit( _NOW_WORD_ ) || _NOW_WORD_ == '+' || _NOW_WORD_ == '-' ) {	// 数字か符号なら即値
		*val = atoi( pData+sData.sp.ptr );
		while( isdigit( _NOW_WORD_ ) || _NOW_WORD_ == '+' || _NOW_WORD_ == '-' ) { sData.sp.ptr++; }	// 数字を読み飛ばす
		return TRUE;
	}
	
	return FALSE;	// 失敗
}


//=============================================================================
//  引数文字列を読み込む
//=============================================================================
BOOL pvns_GetString( char* ret, short kanji, short limit )
{
	short i = 0, j;
	char  str[7];

	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// 空白は読み飛ばす
	
	if( _NOW_WORD_ != '\"' ) { return FALSE; }	// 始めの"を読む
	sData.sp.ptr++;
	*ret = '\0';
	while( _NOW_WORD_ != '\"' ) {				// 最後の"まで読む
		// 改行、終端があったり、上限を超えると失敗
		if( _NOW_WORD_ == '\n' || _NOW_WORD_ == '\0' || i >= limit ) { return FALSE; }
		switch( kanji ) {
			case _STRING_KANJI_:
				if( pvns_GetIndexOfVariable( &j ) ) {	// 変数を文字列化
					sprintf( str, "%d", sData.variable[j] );
					while( *str ) {	// バッファに数字が残っている間続ける
						if( isdigit( str[0] ) ) {
							strcat( ret, "０" );
							*(ret+i+1) += str[0] - '0';	// 数字
						} else {
							strcat( ret, "－" );		// マイナス符号
						}
						i += 2;
						for( j = 0; j < 4; j++ ) { str[j] = str[j+1]; }
					}
				} else {
					if( IsKanji( _NOW_WORD_ ) ) {					// 漢字
						*(ret+i) = _NOW_WORD_;
						sData.sp.ptr++;
						i++;
					}
					*(ret+i) = _NOW_WORD_;
					sData.sp.ptr++;
					i++;
				}
				break;
			case _STRING_FILENAME_:
				if( !IsKanji( _NOW_WORD_ ) ) {
					*(ret+i) = _NOW_WORD_;
					sData.sp.ptr++;
					i++;
				} else {
					return FALSE;
				}
				break;
		}
	}
	sData.sp.ptr++;
	*(ret+i) = '\0';
	return TRUE;
}


//=============================================================================
//  ラベル文字列の比較
//=============================================================================
BOOL pvns_strcmpLabel( unsigned long ptr2 )
{
	unsigned long ptr1 = sData.sp.ptr;
	
	while( *(pData+ptr1) == *(pData+ptr2) ) {
		ptr1++;
		ptr2++;
		if( IsKanji( *(pData+ptr1) ) ) { continue; }
		// カンマ、空白、改行で終端
		if( ( *(pData+ptr1) == ' ' || *(pData+ptr1) == ',' || *(pData+ptr1) == '\n' ) && \
			( *(pData+ptr2) == ' ' || *(pData+ptr2) == '\n' ) ) {
			sData.sp.ptr = ptr1;	// ポインタをラベル終端まで進める
			return TRUE;	// 成功
		}
	}
	
	return FALSE;			// 失敗
}


//=============================================================================
//  ラベルを照合して目的のスクリプトポインタを返す
//=============================================================================
BOOL pvns_GetPointerFromLabel( SCRIPT_POINTER *sp )
{
	short i;
	
	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// 空白は読み飛ばす
	if( _NOW_WORD_ != '*' ) { return FALSE; }		// ラベル指定でなければ失敗
	sp->line = 1;
	// とりあえずラベル記憶に入ってない範囲を総当りで
	for( sp->ptr = _HEADER_LENGTH_; sp->ptr < label[0].ptr; sp->ptr++ ) {
		if( *(pData+sp->ptr) == '\n' ) {
			sp->line++;
			if( *(pData+sp->ptr+1) == '*' ) {
				sp->ptr++;
				if( ( pvns_strcmpLabel(sp->ptr) ) ) { return TRUE; }
			}
		}
	}
	
	for( i = 0; i < _SIZEOF_LABEL_; i++ ) {	// ラベル記憶と比較
		if( !label[i].ptr ) { break; }	// ポインタが空ならループ抜ける
		if( ( pvns_strcmpLabel(label[i].ptr) ) ) {
			*sp = label[i];
			return TRUE;
		}
	}
	
	return FALSE;	// 失敗
}


//=============================================================================
//  コマンド構文解析
//=============================================================================
BOOL pvns_AnalyzeCommand( void )
{
	short i = 0, val[3];
	char str[61];
	SCRIPT_POINTER sp;
	
	// 空白、改行まで命令として読み込む
	while(isalpha(_NOW_WORD_)) {
		*(str+i) = _NOW_WORD_;
		sData.sp.ptr++;
		i++;
	}
	*(str+i) = '\0';
	
/*****************************************************************************/
// 画像系命令
/*****************************************************************************/
	// bg { 0～3 | "filename" }
	if( !strcmp( str, "bg" ) ) {
		pceHeapFree( pgd[_PGD_POS_BG_] );
		pgd[_PGD_POS_BG_] = NULL;
		if( pvns_GetString( str, _STRING_FILENAME_, 16 ) ) {	// ファイル名の場合画像登録
			strcpy(sData.pgdName[_PGD_POS_BG_], str);
			if( ( pgd[_PGD_POS_BG_] = pvns_LoadResourceFile( str, _TYPE_PGD_ ) ) != NULL ) {
				sData.bgColor = _BG_GRAPHIC_;
				textPtr = 0;
				return FALSE;
			}
		} else {
			if( pvns_GetValueOfArgument( &i ) ) {				// 数値の場合背景色登録
				if( i >= 0 && i <= 3 ) {
					sData.pgdName[_PGD_POS_BG_][0] = '\0';
					sData.bgColor = i;
					textPtr = 0;
					return FALSE;
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// ld "filename", pos
	if( !strcmp( str, "ld" ) ) {
		if( pvns_GetString( str, _STRING_FILENAME_, 16 ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &i ) ) {
					if( i >= 0 && i <= 2 ) {
						strcpy( sData.pgdName[i], str );
						pceHeapFree( pgd[i] );
						pgd[i] = NULL;
						if( ( pgd[i] = pvns_LoadResourceFile( str, _TYPE_PGD_ ) ) != NULL ) {
							textPtr = 0;
							return FALSE;
						}
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// cl [ pos ]
	if( !strcmp( str, "cl" ) ) {
		if( pvns_GetValueOfArgument( &i ) ) {	// 引数があればその位置
			if( i >= 0 && i <= 2 ) {
				sData.pgdName[i][0] = '\0';
				pceHeapFree( pgd[i] );
				pgd[i] = NULL;
				textPtr = 0;
				return FALSE;
			}
			/* エラー処理 */
			pvns_SetError(str);
			return FALSE;
		}
		for( i = 0; i <= 2; i++ ) {	// 全部消去
			sData.pgdName[i][0] = '\0';
			pceHeapFree( pgd[i] );
			pgd[i] = NULL;
		}
		textPtr = 0;
		return FALSE;
	}
	// zorder pos1, pos2, pos3（2.0新規）
	if(!strcmp(str, "zorder")) {
		if(pvns_GetValueOfArgument(&val[0])) {
			if(pvns_GetCamma()) {
				if(pvns_GetValueOfArgument(&val[1])) {
					if(pvns_GetCamma()) {
						if(pvns_GetValueOfArgument(&val[2])) {
							if(val[0] != val[1] && val[1] != val[2] && val[2] != val[0]) {
								for( i = 0; i < 3; i++ ) {
									if(val[i] >= 0 && val[i] <= 2) {
										sData.zOrder[i] = val[i];
									}
								}
								textPtr = 0;
								return FALSE;
							}
						}
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// lsp "filename", id, x, y ; lsph "filename", id, x, y
	if( !strcmp( str, "lsp" ) || !strcmp(str, "lsph") ) {
		val[0] = 0;
		if(!strcmp(str, "lsp")) { val[0] = 1; }
		if( pvns_GetString( str, _STRING_FILENAME_, 16 ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &i ) ) {
					if( pvns_GetCamma() ) {
						if( pvns_GetValueOfArgument( &val[1] ) ) {
							if( pvns_GetCamma() ) {
								if( pvns_GetValueOfArgument( &val[2] ) ) {
									if( i >= 0 && i <= 2 ) {
										strcpy( sData.pgdName[_PGD_POS_SP_ + i], str );
										pceHeapFree( pgd[_PGD_POS_SP_ + i] );
										pgd[_PGD_POS_SP_ + i] = NULL;
										if( ( pgd[_PGD_POS_SP_ + i] = pvns_LoadResourceFile( str, _TYPE_PGD_ ) ) != NULL ) {
											sData.sprV[i] = val[0];
											sData.sprX[i] = val[1];
											sData.sprY[i] = val[2];
											
											textPtr = 0;
											return FALSE;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// vsp id, [flag][,[x][,[y]]]
	if(!strcmp(str, "vsp")) {
		if(pvns_GetValueOfArgument(&i)) {
			if(pvns_GetCamma()) {
				if(pvns_GetValueOfArgument(&val[0])) {
					sData.sprV[i] = val[0];
					textPtr = 0;
				}
				if( !pvns_GetCamma() ) { return FALSE; }
				if(pvns_GetValueOfArgument(&val[0])) {
					sData.sprX[i] = val[0];
					textPtr = 0;
				}
				if( !pvns_GetCamma() ) { return FALSE; }
				if(pvns_GetValueOfArgument(&val[0])) {
					sData.sprY[i] = val[0];
					textPtr = 0;
					return FALSE;
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// csp [ id ]
	if(!strcmp(str, "csp")) {
		if(pvns_GetValueOfArgument(&i)) {	// 引数があればその位置
			if(i >= 0 && i <= 2) {
				sData.pgdName[_PGD_POS_SP_ + i][0] = '\0';
				pceHeapFree(pgd[_PGD_POS_SP_ + i]);
				pgd[_PGD_POS_SP_ + i] = NULL;
				sData.sprV[i] = 0;
				textPtr = 0;
				return FALSE;
			}
			/* エラー処理 */
			pvns_SetError(str);
			return FALSE;
		}
		for(i = 0; i < 3; i++) {	// 全部消去
			sData.pgdName[_PGD_POS_SP_ + i][0] = '\0';
			pceHeapFree(pgd[_PGD_POS_SP_ + i]);
			pgd[_PGD_POS_SP_ + i] = NULL;
			sData.sprV[i] = 0;
		}
		textPtr = 0;
		return FALSE;
	}
	
	

/*****************************************************************************/
// 音楽系命令
/*****************************************************************************/
	// snd "filename" [ , ch ]
	if( !strcmp( str, "snd" ) ) {
		if( pvns_GetString( str, _STRING_FILENAME_, 16 ) ) {
			if( pvns_GetCamma() ) {	// 引数があればそのチャネル
				if( pvns_GetValueOfArgument( &i ) ) {
					if( pvns_PlaySound( str, i ) ) { return FALSE; }
				}
				/* エラー処理 */
				pvns_SetError(str);
				return FALSE;
			}
			if( pvns_PlaySound( str, 1 ) ) { return FALSE; }
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// sndstop [ ch ]
	if( !strcmp( str, "sndstop" ) ) {
		if( pvns_GetValueOfArgument( &i ) ) {	// 引数があればそのチャネル
			if( i >= 1 && i <= 3 ) {
				pceWaveAbort( i );
				pceHeapFree( ppd[i] );
				ppd[i] = NULL;
				return FALSE;
			}
			/* エラー処理 */
			pvns_SetError(str);
			return FALSE;
		}	// 引数が無ければ全チャネル停止
		for( i = 1; i <= 3; i++ ) {
			pceWaveAbort( i );
			pceHeapFree( ppd[i] );
			ppd[i] = NULL;
		}
		return FALSE;
	}
	// bgm "filename"
	if( !strcmp( str, "bgm" ) ) {
		if( pvns_GetString( str, _STRING_FILENAME_, 16 ) ) {
			strcpy(sData.pmdName, str);
			StopMusic();
			pceHeapFree( pmd );
			pmd = NULL;
			if( ( pmd = pvns_LoadResourceFile( str, _TYPE_PMD_ ) ) != NULL ) {
				PlayMusic( pmd );
				return FALSE;
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// bgmstop
	if( !strcmp( str, "bgmstop" ) ) {
		sData.pmdName[0] = '\0';
		StopMusic();
		pceHeapFree( pmd );
		pmd = NULL;
		return FALSE;
	}



/*****************************************************************************/
// 演算系命令
/*****************************************************************************/
	// set %1, val
	if( !strcmp( str, "set" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					sData.variable[i] = val[0];
					if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
					return TRUE;
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// inc %1
	if( !strcmp( str, "inc" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			sData.variable[i]++;
			if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// dec %1
	if( !strcmp( str, "dec" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			sData.variable[i]--;
			if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
			return TRUE;
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// rnd %1, upper ; rnd %1, lower, upper
	if( !strcmp( str, "rnd" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2変数
						sData.variable[i] = rand() % val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3変数
						sData.variable[i] = rand() % (val[1]-val[0]) + val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// swap %1, %2
	if( !strcmp( str, "swap" ) ) {
		if( pvns_GetIndexOfVariable( &val[0] ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetIndexOfVariable( &val[1] ) ) {
					i = sData.variable[val[0]];
					sData.variable[val[0]] = sData.variable[val[1]];
					sData.variable[val[1]] = i;
					if(val[0] >= _SIZEOF_LOCAL_ || val[1] >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
					return TRUE;
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// add %1, val ; add %1, val1, val2
	if( !strcmp( str, "add" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2変数
						sData.variable[i] += val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3変数
						sData.variable[i] = val[0] + val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// sub %1, val ; sub %1, val1, val2
	if( !strcmp( str, "sub" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2変数
						sData.variable[i] -= val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3変数
						sData.variable[i] = val[0] - val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// mul %1, val ; add %1, val1, val2
	if( !strcmp( str, "mul" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2変数
						sData.variable[i] *= val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3変数
						sData.variable[i] = val[0] * val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// div %1, val ; add %1, val1, val2
	if( !strcmp( str, "div" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2変数
						sData.variable[i] /= val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3変数
						sData.variable[i] = val[0] / val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// mod %1, val ; add %1, val1, val2
	if( !strcmp( str, "mod" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2変数
						sData.variable[i] %= val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3変数
						sData.variable[i] = val[0] % val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}


/*****************************************************************************/
// 制御系命令1（ジャンプ等）
/*****************************************************************************/
	// br
	if(!strcmp(str, "br")) {
		if(carriageReturn && !(strlen(sData.msg) % _TEXT_LENGTH_)) {
			strcat(sData.msg, "　");
		}
		while(strlen(sData.msg) % _TEXT_LENGTH_) {
			strcat(sData.msg, "　");
		}
		carriageReturn = TRUE;
		return TRUE;
	}
	// goto label
	if( !strcmp( str, "goto" ) ) {
		if( pvns_GetPointerFromLabel(&jump[0]) ) {
			sData.sp = jump[0];
			return TRUE;
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// ongoto value, {*label0}, {*label1}, ...
	if(!strcmp(str, "ongoto")) {
		if(pvns_GetValueOfArgument(&val[0])) {
			for(i = 0; pvns_GetCamma(); i++) {
				if(pvns_GetPointerFromLabel(&jump[0])) {
					if(i == val[0]) {
						sData.sp = jump[0];
						break;
					}
				}
			}
		}
		return TRUE;
	}
	// gosub *label
	if( !strcmp( str, "gosub" ) ) {
		if( pvns_GetPointerFromLabel( &jump[0] ) ) {
			for( i = 0; i < _SIZEOF_STACK_; i++ ) {
				if( !sData.stack[i].ptr ) {
					sData.stack[i] = sData.sp;
					sData.sp = jump[0];
					return TRUE;
					break;
				}
			}
			/* エラー処理：多用 */
			pvns_SetError("stack over");
			return FALSE;
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// ongosub value, {*label0}, {*label1}, ...
	if(!strcmp(str, "ongosub")) {
		jump[0].ptr = 0;
		if(pvns_GetValueOfArgument(&val[0])) {
			for(i = 0; pvns_GetCamma(); i++) {
				if(pvns_GetPointerFromLabel(&jump[0])) {
					if(i == val[0]) { break; }
				}
			}
		}
		if(jump[0].ptr) {
			for(i = 0; i < _SIZEOF_STACK_; i++) {
				if(!sData.stack[i].ptr) {
					sData.stack[i] = sData.sp;
					sData.sp = jump[0];
					return TRUE;
					break;
				}
			}
			/* エラー処理：多用 */
			pvns_SetError("stack over");
			return FALSE;
		}
		return TRUE;
	}
	// return
	if( !strcmp( str, "return" ) ) {
		if( !sData.stack[0].ptr ) {
			/* エラー処理：No Gosub */
			pvns_SetError("no gosub");
			return FALSE;
		}
		for( i = 0; i < _SIZEOF_STACK_ && sData.stack[i].ptr; i++ );
		i--;
		sData.sp = sData.stack[i];
		sData.stack[i].ptr = 0;
		return FALSE;
	}
	// sel "aaa", *1 [ , "bbb", *2 [ , "ccc", *3 ] ]
	if(!strcmp(str, "sel")) {
		sp = sData.sp;
		// 次の行頭へ
		while(strlen(sData.msg) % _TEXT_LENGTH_) { strcat(sData.msg, "　"); }
		// 選択肢数を数える
		for(i = 0; !i || pvns_GetCamma(); i++) {
			if(i <= sData.win.line) {
				if(pvns_GetString(str, _STRING_KANJI_, _TEXT_LENGTH_)) {
					if(pvns_GetCamma()) {
						if(pvns_GetPointerFromLabel(&jump[0])) { continue; }
					}
				}
			}
			/* エラー処理 */
			pvns_SetError(str);
			return FALSE;
			break;
		}
		sData.sp = sp;
		// 選択肢数＞残り行数なら一旦ページクリア
		if(i + strlen(sData.msg) / _TEXT_LENGTH_ > sData.win.line) {
			sData.sp.ptr -= 3;
			_FillMsgWindow;
			return TRUE;
		}
		memset(jump, 0, sizeof(SCRIPT_POINTER)*_SIZEOF_JUMP_);
		for(i = 0; !i || pvns_GetCamma(); i++) {
			pvns_GetString(str, _STRING_KANJI_, _TEXT_LENGTH_);
			strcat(sData.msg, str);
			while(strlen(sData.msg) % _TEXT_LENGTH_) { strcat(sData.msg, "　"); }
			pvns_GetCamma();
			pvns_GetPointerFromLabel(&jump[i]);
		}
		sData.sp= sp;
		sData.sp.ptr -= 3;
		gameMode = _GAMEMODE_SELWAIT_ | _GAMEMODE_LCDINIT_;
		return FALSE;
	}
	// if 条件 実行文
	if( !strcmp( str, "if" ) ) {
		if( pvns_GetValueOfArgument( &val[0] ) ) {
			while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// 空白は読み飛ばす
			// 等号or不等号を読み込む
			i = 0;
			while( _NOW_WORD_ == '=' || _NOW_WORD_ == '<' || _NOW_WORD_ == '>' || _NOW_WORD_ == '!' ) {
				*(str+i) = _NOW_WORD_;
				sData.sp.ptr++;
				i++;
			}
			*(str+i) = '\0';
			if( pvns_GetValueOfArgument( &val[1] ) ) {
				if( !strcmp( str, "=" ) || !strcmp( str, "==" ) ) {
					if( val[0] != val[1] ) { pvns_SkipToLF(); }
					return TRUE;
				}
				if( !strcmp( str, "<>" ) || !strcmp( str, "!=" ) ) {
					if( val[0] == val[1] ) { pvns_SkipToLF(); }
					return TRUE;
				}
				if( !strcmp( str, "<" ) ) {
					if( val[0] >= val[1] ) { pvns_SkipToLF(); }
					return TRUE;
				}
				if( !strcmp( str, ">" ) ) {
					if( val[0] <= val[1] ) { pvns_SkipToLF(); }
					return TRUE;
				}
				if( !strcmp( str, ">=" ) || !strcmp( str, "=>" ) ) {
					if( val[0] < val[1] ) { pvns_SkipToLF(); }
					return TRUE;
				}
				if( !strcmp( str, "<=" ) || !strcmp( str, "=<" ) ) {
					if( val[0] > val[1] ) { pvns_SkipToLF(); }
					return TRUE;
				}
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	
	
	
/*****************************************************************************/
// 制御系命令2（全体の制御）
/*****************************************************************************/
	// wait time [ , flag ]（PVNS2以前=100ms、PVNSCR=10ms）
	if( !strcmp( str, "wait" ) ) {
		if( pvns_GetValueOfArgument( &i ) ) {
			if( i > 0 ) {
				val[0] = 0;
				if( pvns_GetCamma() ) {
					if( !pvns_GetValueOfArgument( &val[0] ) ) {
						/* エラー処理 */
						pvns_SetError(str);
						return FALSE;
					}
				}
				if( _LEGACY_VERSION_ ) { i *= 10; }
				_SetWait( i, ( val[0] != 0 ) );
				return FALSE;
			}
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// savefile "filename"
	if( !strcmp( str, "savefile" ) ) {
		if( !pvns_IsSaveFileExist() ) {
			if( pvns_GetString( str, _STRING_FILENAME_, 16 ) ) {
				if( pvns_SaveFileDefine( str, &sData) ) {
					saveAllow = TRUE;
					return TRUE;
				}
			}
			/* エラー処理：作成失敗 */
			pvns_SetError("savefile create");
			return FALSE;
		}
		/* エラー処理：二重登録 */
		pvns_SetError("savefile define");
		return FALSE;
	}
	// save { 0 | 1 }
	if( !strcmp( str, "save" ) ) {
		if( pvns_GetValueOfArgument( &i ) ) {
			saveAllow = ( i == 0 );	// 0なら不許可、それ以外なら許可
			return TRUE;
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// clickstr "str", line
	if(!strcmp(str, "clickstr")) {
		if(pvns_GetString(sData.clickStr, _STRING_KANJI_, 16)) {
			sData.clickLine = 1;
			if(pvns_GetCamma()) {
				if(!pvns_GetValueOfArgument( &i )) {
					/* エラー処理 */
					pvns_SetError(str);
					return FALSE;
				}
				sData.clickLine = i;
			}
			return TRUE;
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// setwin pos, line, font, color（省略自在）
	if( !strcmp( str, "setwin" ) ) {
		if( sData.msg[0] != '\0' ) {	// メッセージがあればキー待ちページ送り
			sData.sp.ptr -= 6;
			_FillMsgWindow;
			return TRUE;
		}
		if( pvns_GetValueOfArgument( &i ) ) {	// ウィンドウ位置
			if( i < 0 || i > 1 ) {
				/* エラー処理 */
				pvns_SetError(str);
				return FALSE;
			}
			sData.win.pos = i;
		}
		if( !pvns_GetCamma() ) { return FALSE; }
		if( pvns_GetValueOfArgument( &i ) ) {	// ウィンドウ行数
			if( i < 1 || i > 8 ) {
				/* エラー処理 */
				pvns_SetError(str);
				return FALSE;
			}
			sData.win.line = i;
		}
		if( !pvns_GetCamma() ) { return FALSE; }
		if( pvns_GetValueOfArgument( &i ) ) {	// フォント
			if( i < 0 || i > 1 ) {
				/* エラー処理 */
				pvns_SetError(str);
				return FALSE;
			}
			sData.win.font = i;
		}
		if( !pvns_GetCamma() ) { return FALSE; }
		if( pvns_GetValueOfArgument( &i ) ) {	// ウィンドウ色
			if( i < 0 || i > 1 ) {
				/* エラー処理 */
				pvns_SetError(str);
				return FALSE;
			}
			sData.win.color = i;
		}
		return FALSE;
	}
	// mode {0|1}
	if( !strcmp( str, "mode" ) ) {
		if( sData.msg[0] != '\0' ) {	// メッセージがあればキー待ちページ送り
			sData.sp.ptr -= 4;
			_FillMsgWindow;
			return TRUE;
		}
		if( pvns_GetValueOfArgument( &i ) ) {
			sData.win.pos = 1;		// ウィンドウ位置下から
			sData.win.color = 1;	// ウィンドウ色黒
			if( i ) {	// 0以外：全画面
				sData.win.line = 8;
			} else {	// 0	：3行
				sData.win.line = 3;
			}
			return FALSE;
		}
		/* エラー処理 */
		pvns_SetError(str);
		return FALSE;
	}
	// font type（2.0新規）
	if( !strcmp( str, "font" ) ) {
		if( sData.msg[0] != '\0' ) {	// メッセージがあればキー待ちページ送り
			sData.sp.ptr -= 4;
			_FillMsgWindow;
			return TRUE;
		}
		if( pvns_GetValueOfArgument( &i ) ) {
			if( i ) {	// 0以外：縮小フォント
				sData.win.font = 1;
			} else {	// 0	：通常フォント
				sData.win.font = 0;
			}
			return FALSE;
		}
		/* エラー処理 */
		pvns_SetError(str);
		pvns_SetError("font");	//☆
		return FALSE;
	}
	// title
	if(!strcmp(str, "title")) {
		if( sData.msg[0] != '\0' ) {	// メッセージがあればキー待ちページ送り
			sData.sp.ptr -= 5;
			_FillMsgWindow;
			return TRUE;
		}
		gameMode = _GAMEMODE_TITLE_ | _GAMEMODE_LCDINIT_;
		return FALSE;
	}
	// end
	if( !strcmp( str, "end" ) ) {
		if( sData.msg[0] != '\0' ) {	// メッセージがあればキー待ちページ送り
			sData.sp.ptr -= 3;
			_FillMsgWindow;
			return TRUE;
		}
		pceAppReqExit( 0 );	// メッセージがなければ終了
		return FALSE;
	}
	// winoff
	if( !strcmp( str, "winoff" ) ) {
		sData.msg[0] = '\0';
		return TRUE;
	}
	// debug
	if( !strcmp( str, "debug" ) ) {
		sData.debug = TRUE;
		return TRUE;
	}
	
	/* 文法エラー処理 */
	pvns_SetError("syntax");
	sData.sp.ptr++;	// デバッグモード時にきちんと進むようポインタは進める
	return FALSE;
}
