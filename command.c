//=============================================================================
//  �����̃J���}�ǂݍ���
//=============================================================================
BOOL pvns_GetCamma( void )
{
	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// �󔒂͓ǂݔ�΂�
	if( _NOW_WORD_ == ',' ) {
		sData.sp.ptr++;
		return TRUE;	// ����
	}
	
	return FALSE;		// ���s
}


//=============================================================================
//  �X�N���v�g���s���܂œǂݔ�΂��iif���p�j
//=============================================================================
void pvns_SkipToLF(void)
{
	while( _NOW_WORD_ != '\n' ) {
		if( IsKanji( _NOW_WORD_ ) ) { sData.sp.ptr++; }
			sData.sp.ptr++;
		}
}


//=============================================================================
//  �ϐ��̃C���f�b�N�X��Ԃ��i�O���[�o���ϐ���+128�j
//=============================================================================
BOOL pvns_GetIndexOfVariable( short *index )
{
	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// �󔒂͓ǂݔ�΂�
	if( _NOW_WORD_ == '%' ) {	// ���[�J���ϐ�
		sData.sp.ptr++;
		*index = atoi( pData+sData.sp.ptr );
		if( *index < 0 || *index >= _SIZEOF_LOCAL_ ) { return NULL; }
		while( isdigit( _NOW_WORD_ ) ) { sData.sp.ptr++; }	// ������ǂݔ�΂�
		return TRUE;
	}
	if( _NOW_WORD_ == '$' ) {	// �O���[�o���ϐ�
		sData.sp.ptr++;
		*index = atoi( pData+sData.sp.ptr );
		if( *index < 0 || *index >= _SIZEOF_GLOBAL_ ) { return NULL; }
		*index += _SIZEOF_LOCAL_;
		while( isdigit( _NOW_WORD_ ) ) { sData.sp.ptr++; }	// ������ǂݔ�΂�
		return TRUE;
	}
	
	return FALSE;	// ���s
}


//=============================================================================
//  �����̒l�i���l�A�ϐ��j��Ԃ�
//=============================================================================
BOOL pvns_GetValueOfArgument( short *val )
{
	short i;
	
	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// �󔒂͓ǂݔ�΂�
	if( pvns_GetIndexOfVariable( &i ) ) {	// �ϐ��Ȃ�΃C���f�b�N�X����l�擾
		*val = sData.variable[i];
		return TRUE;
	}
	if( isdigit( _NOW_WORD_ ) || _NOW_WORD_ == '+' || _NOW_WORD_ == '-' ) {	// �����������Ȃ瑦�l
		*val = atoi( pData+sData.sp.ptr );
		while( isdigit( _NOW_WORD_ ) || _NOW_WORD_ == '+' || _NOW_WORD_ == '-' ) { sData.sp.ptr++; }	// ������ǂݔ�΂�
		return TRUE;
	}
	
	return FALSE;	// ���s
}


//=============================================================================
//  �����������ǂݍ���
//=============================================================================
BOOL pvns_GetString( char* ret, short kanji, short limit )
{
	short i = 0, j;
	char  str[7];

	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// �󔒂͓ǂݔ�΂�
	
	if( _NOW_WORD_ != '\"' ) { return FALSE; }	// �n�߂�"��ǂ�
	sData.sp.ptr++;
	*ret = '\0';
	while( _NOW_WORD_ != '\"' ) {				// �Ō��"�܂œǂ�
		// ���s�A�I�[����������A����𒴂���Ǝ��s
		if( _NOW_WORD_ == '\n' || _NOW_WORD_ == '\0' || i >= limit ) { return FALSE; }
		switch( kanji ) {
			case _STRING_KANJI_:
				if( pvns_GetIndexOfVariable( &j ) ) {	// �ϐ��𕶎���
					sprintf( str, "%d", sData.variable[j] );
					while( *str ) {	// �o�b�t�@�ɐ������c���Ă���ԑ�����
						if( isdigit( str[0] ) ) {
							strcat( ret, "�O" );
							*(ret+i+1) += str[0] - '0';	// ����
						} else {
							strcat( ret, "�|" );		// �}�C�i�X����
						}
						i += 2;
						for( j = 0; j < 4; j++ ) { str[j] = str[j+1]; }
					}
				} else {
					if( IsKanji( _NOW_WORD_ ) ) {					// ����
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
//  ���x��������̔�r
//=============================================================================
BOOL pvns_strcmpLabel( unsigned long ptr2 )
{
	unsigned long ptr1 = sData.sp.ptr;
	
	while( *(pData+ptr1) == *(pData+ptr2) ) {
		ptr1++;
		ptr2++;
		if( IsKanji( *(pData+ptr1) ) ) { continue; }
		// �J���}�A�󔒁A���s�ŏI�[
		if( ( *(pData+ptr1) == ' ' || *(pData+ptr1) == ',' || *(pData+ptr1) == '\n' ) && \
			( *(pData+ptr2) == ' ' || *(pData+ptr2) == '\n' ) ) {
			sData.sp.ptr = ptr1;	// �|�C���^�����x���I�[�܂Ői�߂�
			return TRUE;	// ����
		}
	}
	
	return FALSE;			// ���s
}


//=============================================================================
//  ���x�����ƍ����ĖړI�̃X�N���v�g�|�C���^��Ԃ�
//=============================================================================
BOOL pvns_GetPointerFromLabel( SCRIPT_POINTER *sp )
{
	short i;
	
	while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// �󔒂͓ǂݔ�΂�
	if( _NOW_WORD_ != '*' ) { return FALSE; }		// ���x���w��łȂ���Ύ��s
	sp->line = 1;
	// �Ƃ肠�������x���L���ɓ����ĂȂ��͈͂𑍓����
	for( sp->ptr = _HEADER_LENGTH_; sp->ptr < label[0].ptr; sp->ptr++ ) {
		if( *(pData+sp->ptr) == '\n' ) {
			sp->line++;
			if( *(pData+sp->ptr+1) == '*' ) {
				sp->ptr++;
				if( ( pvns_strcmpLabel(sp->ptr) ) ) { return TRUE; }
			}
		}
	}
	
	for( i = 0; i < _SIZEOF_LABEL_; i++ ) {	// ���x���L���Ɣ�r
		if( !label[i].ptr ) { break; }	// �|�C���^����Ȃ烋�[�v������
		if( ( pvns_strcmpLabel(label[i].ptr) ) ) {
			*sp = label[i];
			return TRUE;
		}
	}
	
	return FALSE;	// ���s
}


//=============================================================================
//  �R�}���h�\�����
//=============================================================================
BOOL pvns_AnalyzeCommand( void )
{
	short i = 0, val[3];
	char str[61];
	SCRIPT_POINTER sp;
	
	// �󔒁A���s�܂Ŗ��߂Ƃ��ēǂݍ���
	while(isalpha(_NOW_WORD_)) {
		*(str+i) = _NOW_WORD_;
		sData.sp.ptr++;
		i++;
	}
	*(str+i) = '\0';
	
/*****************************************************************************/
// �摜�n����
/*****************************************************************************/
	// bg { 0�`3 | "filename" }
	if( !strcmp( str, "bg" ) ) {
		pceHeapFree( pgd[_PGD_POS_BG_] );
		pgd[_PGD_POS_BG_] = NULL;
		if( pvns_GetString( str, _STRING_FILENAME_, 16 ) ) {	// �t�@�C�����̏ꍇ�摜�o�^
			strcpy(sData.pgdName[_PGD_POS_BG_], str);
			if( ( pgd[_PGD_POS_BG_] = pvns_LoadResourceFile( str, _TYPE_PGD_ ) ) != NULL ) {
				sData.bgColor = _BG_GRAPHIC_;
				textPtr = 0;
				return FALSE;
			}
		} else {
			if( pvns_GetValueOfArgument( &i ) ) {				// ���l�̏ꍇ�w�i�F�o�^
				if( i >= 0 && i <= 3 ) {
					sData.pgdName[_PGD_POS_BG_][0] = '\0';
					sData.bgColor = i;
					textPtr = 0;
					return FALSE;
				}
			}
		}
		/* �G���[���� */
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
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// cl [ pos ]
	if( !strcmp( str, "cl" ) ) {
		if( pvns_GetValueOfArgument( &i ) ) {	// ����������΂��̈ʒu
			if( i >= 0 && i <= 2 ) {
				sData.pgdName[i][0] = '\0';
				pceHeapFree( pgd[i] );
				pgd[i] = NULL;
				textPtr = 0;
				return FALSE;
			}
			/* �G���[���� */
			pvns_SetError(str);
			return FALSE;
		}
		for( i = 0; i <= 2; i++ ) {	// �S������
			sData.pgdName[i][0] = '\0';
			pceHeapFree( pgd[i] );
			pgd[i] = NULL;
		}
		textPtr = 0;
		return FALSE;
	}
	// zorder pos1, pos2, pos3�i2.0�V�K�j
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
		/* �G���[���� */
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
		/* �G���[���� */
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
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// csp [ id ]
	if(!strcmp(str, "csp")) {
		if(pvns_GetValueOfArgument(&i)) {	// ����������΂��̈ʒu
			if(i >= 0 && i <= 2) {
				sData.pgdName[_PGD_POS_SP_ + i][0] = '\0';
				pceHeapFree(pgd[_PGD_POS_SP_ + i]);
				pgd[_PGD_POS_SP_ + i] = NULL;
				sData.sprV[i] = 0;
				textPtr = 0;
				return FALSE;
			}
			/* �G���[���� */
			pvns_SetError(str);
			return FALSE;
		}
		for(i = 0; i < 3; i++) {	// �S������
			sData.pgdName[_PGD_POS_SP_ + i][0] = '\0';
			pceHeapFree(pgd[_PGD_POS_SP_ + i]);
			pgd[_PGD_POS_SP_ + i] = NULL;
			sData.sprV[i] = 0;
		}
		textPtr = 0;
		return FALSE;
	}
	
	

/*****************************************************************************/
// ���y�n����
/*****************************************************************************/
	// snd "filename" [ , ch ]
	if( !strcmp( str, "snd" ) ) {
		if( pvns_GetString( str, _STRING_FILENAME_, 16 ) ) {
			if( pvns_GetCamma() ) {	// ����������΂��̃`���l��
				if( pvns_GetValueOfArgument( &i ) ) {
					if( pvns_PlaySound( str, i ) ) { return FALSE; }
				}
				/* �G���[���� */
				pvns_SetError(str);
				return FALSE;
			}
			if( pvns_PlaySound( str, 1 ) ) { return FALSE; }
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// sndstop [ ch ]
	if( !strcmp( str, "sndstop" ) ) {
		if( pvns_GetValueOfArgument( &i ) ) {	// ����������΂��̃`���l��
			if( i >= 1 && i <= 3 ) {
				pceWaveAbort( i );
				pceHeapFree( ppd[i] );
				ppd[i] = NULL;
				return FALSE;
			}
			/* �G���[���� */
			pvns_SetError(str);
			return FALSE;
		}	// ������������ΑS�`���l����~
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
		/* �G���[���� */
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
// ���Z�n����
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
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// inc %1
	if( !strcmp( str, "inc" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			sData.variable[i]++;
			if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
		}
		/* �G���[���� */
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
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// rnd %1, upper ; rnd %1, lower, upper
	if( !strcmp( str, "rnd" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2�ϐ�
						sData.variable[i] = rand() % val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3�ϐ�
						sData.variable[i] = rand() % (val[1]-val[0]) + val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* �G���[���� */
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
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// add %1, val ; add %1, val1, val2
	if( !strcmp( str, "add" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2�ϐ�
						sData.variable[i] += val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3�ϐ�
						sData.variable[i] = val[0] + val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// sub %1, val ; sub %1, val1, val2
	if( !strcmp( str, "sub" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2�ϐ�
						sData.variable[i] -= val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3�ϐ�
						sData.variable[i] = val[0] - val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// mul %1, val ; add %1, val1, val2
	if( !strcmp( str, "mul" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2�ϐ�
						sData.variable[i] *= val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3�ϐ�
						sData.variable[i] = val[0] * val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// div %1, val ; add %1, val1, val2
	if( !strcmp( str, "div" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2�ϐ�
						sData.variable[i] /= val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3�ϐ�
						sData.variable[i] = val[0] / val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// mod %1, val ; add %1, val1, val2
	if( !strcmp( str, "mod" ) ) {
		if( pvns_GetIndexOfVariable( &i ) ) {
			if( pvns_GetCamma() ) {
				if( pvns_GetValueOfArgument( &val[0] ) ) {
					if( !pvns_GetCamma() ) {					// 2�ϐ�
						sData.variable[i] %= val[0];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
					if( pvns_GetValueOfArgument( &val[1] ) ) {	// 3�ϐ�
						sData.variable[i] = val[0] % val[1];
						if(i >= _SIZEOF_LOCAL_) { pvns_GlobalSave(&sData); }
						return TRUE;
					}
				}
			}
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}


/*****************************************************************************/
// ����n����1�i�W�����v���j
/*****************************************************************************/
	// br
	if(!strcmp(str, "br")) {
		if(carriageReturn && !(strlen(sData.msg) % _TEXT_LENGTH_)) {
			strcat(sData.msg, "�@");
		}
		while(strlen(sData.msg) % _TEXT_LENGTH_) {
			strcat(sData.msg, "�@");
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
		/* �G���[���� */
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
			/* �G���[�����F���p */
			pvns_SetError("stack over");
			return FALSE;
		}
		/* �G���[���� */
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
			/* �G���[�����F���p */
			pvns_SetError("stack over");
			return FALSE;
		}
		return TRUE;
	}
	// return
	if( !strcmp( str, "return" ) ) {
		if( !sData.stack[0].ptr ) {
			/* �G���[�����FNo Gosub */
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
		// ���̍s����
		while(strlen(sData.msg) % _TEXT_LENGTH_) { strcat(sData.msg, "�@"); }
		// �I�������𐔂���
		for(i = 0; !i || pvns_GetCamma(); i++) {
			if(i <= sData.win.line) {
				if(pvns_GetString(str, _STRING_KANJI_, _TEXT_LENGTH_)) {
					if(pvns_GetCamma()) {
						if(pvns_GetPointerFromLabel(&jump[0])) { continue; }
					}
				}
			}
			/* �G���[���� */
			pvns_SetError(str);
			return FALSE;
			break;
		}
		sData.sp = sp;
		// �I���������c��s���Ȃ��U�y�[�W�N���A
		if(i + strlen(sData.msg) / _TEXT_LENGTH_ > sData.win.line) {
			sData.sp.ptr -= 3;
			_FillMsgWindow;
			return TRUE;
		}
		memset(jump, 0, sizeof(SCRIPT_POINTER)*_SIZEOF_JUMP_);
		for(i = 0; !i || pvns_GetCamma(); i++) {
			pvns_GetString(str, _STRING_KANJI_, _TEXT_LENGTH_);
			strcat(sData.msg, str);
			while(strlen(sData.msg) % _TEXT_LENGTH_) { strcat(sData.msg, "�@"); }
			pvns_GetCamma();
			pvns_GetPointerFromLabel(&jump[i]);
		}
		sData.sp= sp;
		sData.sp.ptr -= 3;
		gameMode = _GAMEMODE_SELWAIT_ | _GAMEMODE_LCDINIT_;
		return FALSE;
	}
	// if ���� ���s��
	if( !strcmp( str, "if" ) ) {
		if( pvns_GetValueOfArgument( &val[0] ) ) {
			while( _NOW_WORD_ == ' ' ) { sData.sp.ptr++; }	// �󔒂͓ǂݔ�΂�
			// ����or�s������ǂݍ���
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
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	
	
	
/*****************************************************************************/
// ����n����2�i�S�̂̐���j
/*****************************************************************************/
	// wait time [ , flag ]�iPVNS2�ȑO=100ms�APVNSCR=10ms�j
	if( !strcmp( str, "wait" ) ) {
		if( pvns_GetValueOfArgument( &i ) ) {
			if( i > 0 ) {
				val[0] = 0;
				if( pvns_GetCamma() ) {
					if( !pvns_GetValueOfArgument( &val[0] ) ) {
						/* �G���[���� */
						pvns_SetError(str);
						return FALSE;
					}
				}
				if( _LEGACY_VERSION_ ) { i *= 10; }
				_SetWait( i, ( val[0] != 0 ) );
				return FALSE;
			}
		}
		/* �G���[���� */
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
			/* �G���[�����F�쐬���s */
			pvns_SetError("savefile create");
			return FALSE;
		}
		/* �G���[�����F��d�o�^ */
		pvns_SetError("savefile define");
		return FALSE;
	}
	// save { 0 | 1 }
	if( !strcmp( str, "save" ) ) {
		if( pvns_GetValueOfArgument( &i ) ) {
			saveAllow = ( i == 0 );	// 0�Ȃ�s���A����ȊO�Ȃ狖��
			return TRUE;
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// clickstr "str", line
	if(!strcmp(str, "clickstr")) {
		if(pvns_GetString(sData.clickStr, _STRING_KANJI_, 16)) {
			sData.clickLine = 1;
			if(pvns_GetCamma()) {
				if(!pvns_GetValueOfArgument( &i )) {
					/* �G���[���� */
					pvns_SetError(str);
					return FALSE;
				}
				sData.clickLine = i;
			}
			return TRUE;
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// setwin pos, line, font, color�i�ȗ����݁j
	if( !strcmp( str, "setwin" ) ) {
		if( sData.msg[0] != '\0' ) {	// ���b�Z�[�W������΃L�[�҂��y�[�W����
			sData.sp.ptr -= 6;
			_FillMsgWindow;
			return TRUE;
		}
		if( pvns_GetValueOfArgument( &i ) ) {	// �E�B���h�E�ʒu
			if( i < 0 || i > 1 ) {
				/* �G���[���� */
				pvns_SetError(str);
				return FALSE;
			}
			sData.win.pos = i;
		}
		if( !pvns_GetCamma() ) { return FALSE; }
		if( pvns_GetValueOfArgument( &i ) ) {	// �E�B���h�E�s��
			if( i < 1 || i > 8 ) {
				/* �G���[���� */
				pvns_SetError(str);
				return FALSE;
			}
			sData.win.line = i;
		}
		if( !pvns_GetCamma() ) { return FALSE; }
		if( pvns_GetValueOfArgument( &i ) ) {	// �t�H���g
			if( i < 0 || i > 1 ) {
				/* �G���[���� */
				pvns_SetError(str);
				return FALSE;
			}
			sData.win.font = i;
		}
		if( !pvns_GetCamma() ) { return FALSE; }
		if( pvns_GetValueOfArgument( &i ) ) {	// �E�B���h�E�F
			if( i < 0 || i > 1 ) {
				/* �G���[���� */
				pvns_SetError(str);
				return FALSE;
			}
			sData.win.color = i;
		}
		return FALSE;
	}
	// mode {0|1}
	if( !strcmp( str, "mode" ) ) {
		if( sData.msg[0] != '\0' ) {	// ���b�Z�[�W������΃L�[�҂��y�[�W����
			sData.sp.ptr -= 4;
			_FillMsgWindow;
			return TRUE;
		}
		if( pvns_GetValueOfArgument( &i ) ) {
			sData.win.pos = 1;		// �E�B���h�E�ʒu������
			sData.win.color = 1;	// �E�B���h�E�F��
			if( i ) {	// 0�ȊO�F�S���
				sData.win.line = 8;
			} else {	// 0	�F3�s
				sData.win.line = 3;
			}
			return FALSE;
		}
		/* �G���[���� */
		pvns_SetError(str);
		return FALSE;
	}
	// font type�i2.0�V�K�j
	if( !strcmp( str, "font" ) ) {
		if( sData.msg[0] != '\0' ) {	// ���b�Z�[�W������΃L�[�҂��y�[�W����
			sData.sp.ptr -= 4;
			_FillMsgWindow;
			return TRUE;
		}
		if( pvns_GetValueOfArgument( &i ) ) {
			if( i ) {	// 0�ȊO�F�k���t�H���g
				sData.win.font = 1;
			} else {	// 0	�F�ʏ�t�H���g
				sData.win.font = 0;
			}
			return FALSE;
		}
		/* �G���[���� */
		pvns_SetError(str);
		pvns_SetError("font");	//��
		return FALSE;
	}
	// title
	if(!strcmp(str, "title")) {
		if( sData.msg[0] != '\0' ) {	// ���b�Z�[�W������΃L�[�҂��y�[�W����
			sData.sp.ptr -= 5;
			_FillMsgWindow;
			return TRUE;
		}
		gameMode = _GAMEMODE_TITLE_ | _GAMEMODE_LCDINIT_;
		return FALSE;
	}
	// end
	if( !strcmp( str, "end" ) ) {
		if( sData.msg[0] != '\0' ) {	// ���b�Z�[�W������΃L�[�҂��y�[�W����
			sData.sp.ptr -= 3;
			_FillMsgWindow;
			return TRUE;
		}
		pceAppReqExit( 0 );	// ���b�Z�[�W���Ȃ���ΏI��
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
	
	/* ���@�G���[���� */
	pvns_SetError("syntax");
	sData.sp.ptr++;	// �f�o�b�O���[�h���ɂ�����Ɛi�ނ悤�|�C���^�͐i�߂�
	return FALSE;
}
